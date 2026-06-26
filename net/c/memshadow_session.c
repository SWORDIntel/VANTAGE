/**
 * MEMSHADOW Protocol v3.0 - Session Management Implementation (C)
 */

#include "memshadow_session.h"
#include "memshadow.h"
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/random.h>
#endif

static int session_random(uint8_t *buf, size_t len) {
#ifdef __linux__
    return (getrandom(buf, len, 0) == (ssize_t)len) ? 0 : -1;
#else
    for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)(rand() & 0xFF);
    return 0;
#endif
}

void memshadow_session_manager_init(memshadow_session_manager_t *mgr, uint64_t timeout_ms) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->default_timeout_ms = (timeout_ms > 0) ? timeout_ms : 30000;
    mgr->key_rotation_interval_ms = 3600000; /* 1 hour */
}

int memshadow_session_create(memshadow_session_manager_t *mgr,
                             const char *peer_id,
                             const uint8_t *session_key,
                             const uint8_t *hmac_key,
                             char *out_session_id) {
    if (!mgr || !peer_id || !session_key || !hmac_key) return -1;

    if (mgr->session_count >= MEMSHADOW_SESSION_MAX) {
        memshadow_session_cleanup_expired(mgr);
        if (mgr->session_count >= MEMSHADOW_SESSION_MAX) return -2;
    }

    memshadow_session_t *s = &mgr->sessions[mgr->session_count];
    memset(s, 0, sizeof(*s));

    /* Generate session ID (16 random bytes → 32 hex chars) */
    uint8_t id_bytes[16];
    if (session_random(id_bytes, 16) != 0) return -3;
    for (int i = 0; i < 16; i++)
        snprintf(s->session_id + i * 2, 3, "%02x", id_bytes[i]);

    strncpy(s->peer_id, peer_id, sizeof(s->peer_id) - 1);
    s->state = SESSION_ACTIVE;
    s->created_at_ns = memshadow_get_timestamp_ns();
    s->last_activity_ns = s->created_at_ns;
    s->timeout_ms = mgr->default_timeout_ms;
    memcpy(s->session_key, session_key, MEMSHADOW_SESSION_KEY_SIZE);
    memcpy(s->hmac_key, hmac_key, MEMSHADOW_SESSION_KEY_SIZE);

    if (out_session_id)
        strcpy(out_session_id, s->session_id);

    mgr->session_count++;
    return 0;
}

memshadow_session_t *memshadow_session_find(memshadow_session_manager_t *mgr,
                                             const char *session_id) {
    if (!mgr || !session_id) return NULL;
    for (uint32_t i = 0; i < mgr->session_count; i++) {
        if (strcmp(mgr->sessions[i].session_id, session_id) == 0)
            return &mgr->sessions[i];
    }
    return NULL;
}

memshadow_session_t *memshadow_session_find_by_peer(memshadow_session_manager_t *mgr,
                                                     const char *peer_id) {
    if (!mgr || !peer_id) return NULL;
    for (uint32_t i = 0; i < mgr->session_count; i++) {
        if (mgr->sessions[i].state == SESSION_ACTIVE &&
            strcmp(mgr->sessions[i].peer_id, peer_id) == 0)
            return &mgr->sessions[i];
    }
    return NULL;
}

int memshadow_session_close(memshadow_session_manager_t *mgr, const char *session_id) {
    memshadow_session_t *s = memshadow_session_find(mgr, session_id);
    if (!s) return -1;
    s->state = SESSION_CLOSING;
    return 0;
}

void memshadow_session_record_send(memshadow_session_t *session, uint64_t bytes) {
    if (!session) return;
    session->messages_sent++;
    session->bytes_sent += bytes;
    session->last_activity_ns = memshadow_get_timestamp_ns();
    if (session->state == SESSION_IDLE) session->state = SESSION_ACTIVE;
}

void memshadow_session_record_receive(memshadow_session_t *session, uint64_t bytes) {
    if (!session) return;
    session->messages_received++;
    session->bytes_received += bytes;
    session->last_activity_ns = memshadow_get_timestamp_ns();
    if (session->state == SESSION_IDLE) session->state = SESSION_ACTIVE;
}

int memshadow_session_rotate_key(memshadow_session_t *session,
                                  const uint8_t *new_key,
                                  const uint8_t *new_hmac) {
    if (!session || !new_key || !new_hmac) return -1;
    memcpy(session->session_key, new_key, MEMSHADOW_SESSION_KEY_SIZE);
    memcpy(session->hmac_key, new_hmac, MEMSHADOW_SESSION_KEY_SIZE);
    session->key_rotation_count++;
    session->last_activity_ns = memshadow_get_timestamp_ns();
    return 0;
}

void memshadow_session_tick(memshadow_session_manager_t *mgr) {
    if (!mgr) return;
    uint64_t now = memshadow_get_timestamp_ns();

    for (uint32_t i = 0; i < mgr->session_count; i++) {
        memshadow_session_t *s = &mgr->sessions[i];
        uint64_t idle_ms = (now - s->last_activity_ns) / 1000000ULL;

        if (s->state == SESSION_ACTIVE && idle_ms > s->timeout_ms / 2) {
            s->state = SESSION_IDLE;
        }
        if (idle_ms > s->timeout_ms) {
            s->state = SESSION_EXPIRED;
        }
    }
}

void memshadow_session_cleanup_expired(memshadow_session_manager_t *mgr) {
    if (!mgr) return;
    uint64_t now = memshadow_get_timestamp_ns();
    uint32_t write_idx = 0;

    for (uint32_t i = 0; i < mgr->session_count; i++) {
        uint64_t idle_ms = (now - mgr->sessions[i].last_activity_ns) / 1000000ULL;
        bool remove = (mgr->sessions[i].state == SESSION_EXPIRED && idle_ms > 300000);
        if (!remove) {
            if (write_idx != i) mgr->sessions[write_idx] = mgr->sessions[i];
            write_idx++;
        } else {
            /* Zero out key material */
            memset(mgr->sessions[i].session_key, 0, MEMSHADOW_SESSION_KEY_SIZE);
            memset(mgr->sessions[i].hmac_key, 0, MEMSHADOW_SESSION_KEY_SIZE);
        }
    }
    mgr->session_count = write_idx;
}

void memshadow_session_get_stats(const memshadow_session_manager_t *mgr,
                                  memshadow_session_stats_t *stats) {
    if (!mgr || !stats) return;
    memset(stats, 0, sizeof(*stats));
    for (uint32_t i = 0; i < mgr->session_count; i++) {
        const memshadow_session_t *s = &mgr->sessions[i];
        switch (s->state) {
            case SESSION_ACTIVE: stats->active++; break;
            case SESSION_IDLE: stats->idle++; break;
            case SESSION_EXPIRED: stats->expired++; break;
            case SESSION_CLOSING: stats->closing++; break;
        }
        stats->total_messages_sent += s->messages_sent;
        stats->total_messages_received += s->messages_received;
        stats->total_bytes_sent += s->bytes_sent;
        stats->total_bytes_received += s->bytes_received;
    }
}

uint32_t memshadow_session_active_count(const memshadow_session_manager_t *mgr) {
    if (!mgr) return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < mgr->session_count; i++) {
        if (mgr->sessions[i].state == SESSION_ACTIVE) count++;
    }
    return count;
}
