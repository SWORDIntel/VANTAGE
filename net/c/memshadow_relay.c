/**
 * MEMSHADOW Protocol v3.0 - Relay System Implementation (C)
 */

#include "memshadow_relay.h"
#include "memshadow.h"
#include <string.h>

void memshadow_relay_init(memshadow_relay_manager_t *mgr, const char *node_id) {
    memset(mgr, 0, sizeof(*mgr));
    strncpy(mgr->node_id, node_id, sizeof(mgr->node_id) - 1);
    mgr->max_hops = 5;
    mgr->route_ttl_ms = 3600000;
}

int memshadow_relay_add_route(memshadow_relay_manager_t *mgr, const memshadow_relay_route_t *route) {
    if (!mgr || !route || route->hop_count == 0) return -1;

    /* Check if we already have routes to this destination */
    int alt_count = 0;
    int worst_idx = -1;
    double worst_latency = 0.0;
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        if (strcmp(mgr->routes[i].destination, route->destination) == 0) {
            alt_count++;
            if (mgr->routes[i].total_latency_ms > worst_latency) {
                worst_latency = mgr->routes[i].total_latency_ms;
                worst_idx = (int)i;
            }
        }
    }

    if (alt_count >= MEMSHADOW_RELAY_MAX_ALT) {
        if (worst_idx >= 0 && worst_latency > route->total_latency_ms) {
            mgr->routes[worst_idx] = *route;
            mgr->stats.routes_discovered++;
            return 0;
        }
        return -2; /* All alternatives are better */
    }

    if (mgr->route_count >= MEMSHADOW_RELAY_MAX_ROUTES) {
        memshadow_relay_cleanup(mgr);
        if (mgr->route_count >= MEMSHADOW_RELAY_MAX_ROUTES) return -3;
    }

    mgr->routes[mgr->route_count++] = *route;
    mgr->stats.routes_discovered++;
    return 0;
}

const memshadow_relay_route_t *memshadow_relay_get_route(memshadow_relay_manager_t *mgr, const char *destination) {
    if (!mgr || !destination) return NULL;
    uint64_t now = memshadow_get_timestamp_ns();

    const memshadow_relay_route_t *best = NULL;
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        if (strcmp(mgr->routes[i].destination, destination) != 0) continue;
        uint64_t age_ms = (now - mgr->routes[i].created_at_ns) / 1000000ULL;
        if (age_ms > mgr->routes[i].ttl_ms) continue;
        if (!best || mgr->routes[i].total_latency_ms < best->total_latency_ms) {
            best = &mgr->routes[i];
        }
    }

    if (best) {
        mgr->stats.cache_hits++;
    } else {
        mgr->stats.cache_misses++;
    }
    return best;
}

bool memshadow_relay_has_route(const memshadow_relay_manager_t *mgr, const char *destination) {
    if (!mgr || !destination) return false;
    uint64_t now = memshadow_get_timestamp_ns();
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        if (strcmp(mgr->routes[i].destination, destination) != 0) continue;
        uint64_t age_ms = (now - mgr->routes[i].created_at_ns) / 1000000ULL;
        if (age_ms <= mgr->routes[i].ttl_ms) return true;
    }
    return false;
}

void memshadow_relay_record_success(memshadow_relay_manager_t *mgr, const char *destination, uint64_t bytes) {
    if (!mgr) return;
    mgr->stats.messages_relayed++;
    mgr->stats.bytes_relayed += bytes;
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        if (strcmp(mgr->routes[i].destination, destination) == 0) {
            mgr->routes[i].use_count++;
            break;
        }
    }
}

void memshadow_relay_record_failure(memshadow_relay_manager_t *mgr, const char *destination) {
    if (!mgr) return;
    mgr->stats.relay_failures++;
    /* Remove worst route for this destination */
    int worst_idx = -1;
    double worst_latency = 0.0;
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        if (strcmp(mgr->routes[i].destination, destination) == 0) {
            if (mgr->routes[i].total_latency_ms > worst_latency) {
                worst_latency = mgr->routes[i].total_latency_ms;
                worst_idx = (int)i;
            }
        }
    }
    if (worst_idx >= 0) {
        memmove(&mgr->routes[worst_idx], &mgr->routes[worst_idx + 1],
                (mgr->route_count - worst_idx - 1) * sizeof(memshadow_relay_route_t));
        mgr->route_count--;
    }
}

void memshadow_relay_cleanup(memshadow_relay_manager_t *mgr) {
    if (!mgr) return;
    uint64_t now = memshadow_get_timestamp_ns();
    uint16_t write_idx = 0;
    for (uint16_t i = 0; i < mgr->route_count; i++) {
        uint64_t age_ms = (now - mgr->routes[i].created_at_ns) / 1000000ULL;
        if (age_ms <= mgr->routes[i].ttl_ms) {
            if (write_idx != i) mgr->routes[write_idx] = mgr->routes[i];
            write_idx++;
        } else {
            mgr->stats.routes_expired++;
        }
    }
    mgr->route_count = write_idx;
}

uint16_t memshadow_relay_cached_count(const memshadow_relay_manager_t *mgr) {
    return mgr ? mgr->route_count : 0;
}

void memshadow_relay_get_stats(const memshadow_relay_manager_t *mgr, memshadow_relay_stats_t *stats) {
    if (mgr && stats) *stats = mgr->stats;
}
