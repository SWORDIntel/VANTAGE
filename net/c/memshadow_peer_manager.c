/**
 * MEMSHADOW Protocol v3.0 - Peer Manager Implementation (C)
 */

#include "memshadow_peer_manager.h"
#include "memshadow.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <sys/random.h>
#endif

void memshadow_peer_manager_init(memshadow_peer_manager_t *mgr, uint16_t max_peers) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->max_peers = (max_peers > 0 && max_peers <= MEMSHADOW_PEER_MAX) ? max_peers : MEMSHADOW_PEER_MAX;
    mgr->ping_interval_ms = 10000;
    mgr->ban_duration_ms = 3600000;
    mgr->strategy = SELECT_BEST_HEALTH;
}

int memshadow_peer_add(memshadow_peer_manager_t *mgr, const memshadow_peer_info_t *peer) {
    if (!mgr || !peer) return -1;
    /* Update if exists */
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (strcmp(mgr->peers[i].node_id, peer->node_id) == 0) {
            mgr->peers[i] = *peer;
            return 0;
        }
    }
    if (mgr->peer_count >= mgr->max_peers) return -2;
    mgr->peers[mgr->peer_count++] = *peer;
    return 0;
}

int memshadow_peer_remove(memshadow_peer_manager_t *mgr, const char *node_id) {
    if (!mgr || !node_id) return -1;
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (strcmp(mgr->peers[i].node_id, node_id) == 0) {
            memmove(&mgr->peers[i], &mgr->peers[i + 1],
                    (mgr->peer_count - i - 1) * sizeof(memshadow_peer_info_t));
            mgr->peer_count--;
            return 0;
        }
    }
    return -2;
}

memshadow_peer_info_t *memshadow_peer_find(memshadow_peer_manager_t *mgr, const char *node_id) {
    if (!mgr || !node_id) return NULL;
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (strcmp(mgr->peers[i].node_id, node_id) == 0) return &mgr->peers[i];
    }
    return NULL;
}

double memshadow_peer_health_score(const memshadow_peer_health_t *health) {
    if (!health) return 0.0;
    double latency_score = 1.0 / (1.0 + health->latency_ms / 100.0);
    double loss_score = 1.0 - health->packet_loss;
    double bw_score = (health->bandwidth_bps > 0) ? log2((double)health->bandwidth_bps) / 30.0 : 0.0;
    if (bw_score < 0.0) bw_score = 0.0;
    return latency_score * 0.4 + loss_score * 0.4 + bw_score * 0.2;
}

bool memshadow_peer_is_healthy(const memshadow_peer_health_t *health) {
    if (!health) return false;
    return health->ping_failures < health->max_ping_failures && health->packet_loss < 0.5;
}

memshadow_peer_info_t *memshadow_peer_select(memshadow_peer_manager_t *mgr) {
    if (!mgr || mgr->peer_count == 0) return NULL;

    /* Collect connected + healthy indices */
    uint16_t candidates[MEMSHADOW_PEER_MAX];
    uint16_t cand_count = 0;
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (mgr->peers[i].status == PEER_CONNECTED && memshadow_peer_is_healthy(&mgr->peers[i].health)) {
            candidates[cand_count++] = i;
        }
    }
    if (cand_count == 0) return NULL;

    uint16_t selected = 0;
    switch (mgr->strategy) {
        case SELECT_BEST_HEALTH: {
            double best = -1.0;
            for (uint16_t c = 0; c < cand_count; c++) {
                double score = memshadow_peer_health_score(&mgr->peers[candidates[c]].health);
                if (score > best) { best = score; selected = candidates[c]; }
            }
            break;
        }
        case SELECT_LOWEST_LATENCY: {
            double best = 1e18;
            for (uint16_t c = 0; c < cand_count; c++) {
                double lat = mgr->peers[candidates[c]].health.latency_ms;
                if (lat < best) { best = lat; selected = candidates[c]; }
            }
            break;
        }
        case SELECT_HIGHEST_BANDWIDTH: {
            uint64_t best = 0;
            for (uint16_t c = 0; c < cand_count; c++) {
                uint64_t bw = mgr->peers[candidates[c]].health.bandwidth_bps;
                if (bw > best) { best = bw; selected = candidates[c]; }
            }
            break;
        }
        case SELECT_ROUND_ROBIN:
            mgr->round_robin_index = (mgr->round_robin_index + 1) % cand_count;
            selected = candidates[mgr->round_robin_index];
            break;
        case SELECT_RANDOM: {
            uint8_t rng;
#ifdef __linux__
            getrandom(&rng, 1, 0);
#else
            rng = (uint8_t)(rand() & 0xFF);
#endif
            selected = candidates[rng % cand_count];
            break;
        }
    }
    return &mgr->peers[selected];
}

void memshadow_peer_connect(memshadow_peer_manager_t *mgr, const char *node_id) {
    memshadow_peer_info_t *p = memshadow_peer_find(mgr, node_id);
    if (p) { p->status = PEER_CONNECTED; p->connected_since_ns = memshadow_get_timestamp_ns(); }
}

void memshadow_peer_disconnect(memshadow_peer_manager_t *mgr, const char *node_id) {
    memshadow_peer_info_t *p = memshadow_peer_find(mgr, node_id);
    if (p) { p->status = PEER_DISCONNECTED; p->connected_since_ns = 0; }
}

void memshadow_peer_ban(memshadow_peer_manager_t *mgr, const char *node_id) {
    memshadow_peer_info_t *p = memshadow_peer_find(mgr, node_id);
    if (p) { p->status = PEER_BANNED; p->connected_since_ns = 0; }
}

void memshadow_peer_record_ping(memshadow_peer_info_t *peer, double latency_ms) {
    if (!peer) return;
    peer->health.latency_ms = peer->health.latency_ms * 0.7 + latency_ms * 0.3;
    peer->health.last_ping_ns = memshadow_get_timestamp_ns();
    peer->health.ping_failures = 0;
}

void memshadow_peer_record_ping_failure(memshadow_peer_info_t *peer) {
    if (peer) peer->health.ping_failures++;
}

void memshadow_peer_tick(memshadow_peer_manager_t *mgr) {
    if (!mgr) return;
    uint64_t now = memshadow_get_timestamp_ns();
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (mgr->peers[i].status == PEER_CONNECTED) {
            uint64_t since_ping_ms = (now - mgr->peers[i].health.last_ping_ns) / 1000000ULL;
            if (since_ping_ms > mgr->ping_interval_ms && !memshadow_peer_is_healthy(&mgr->peers[i].health)) {
                mgr->peers[i].status = PEER_UNREACHABLE;
            }
        }
    }
}

uint16_t memshadow_peer_connected_count(const memshadow_peer_manager_t *mgr) {
    if (!mgr) return 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < mgr->peer_count; i++) {
        if (mgr->peers[i].status == PEER_CONNECTED) count++;
    }
    return count;
}
