/**
 * MEMSHADOW Protocol v3.0 - Session Management (C)
 *
 * Session lifecycle, key rotation, and timeout management.
 */

#ifndef MEMSHADOW_SESSION_H
#define MEMSHADOW_SESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMSHADOW_SESSION_ID_SIZE    32
#define MEMSHADOW_SESSION_KEY_SIZE   32
#define MEMSHADOW_SESSION_MAX        1024

typedef enum {
    SESSION_ACTIVE = 0,
    SESSION_IDLE = 1,
    SESSION_EXPIRED = 2,
    SESSION_CLOSING = 3,
} memshadow_session_state_t;

typedef struct {
    char     session_id[MEMSHADOW_SESSION_ID_SIZE + 1];
    char     peer_id[64];
    memshadow_session_state_t state;
    uint64_t created_at_ns;
    uint64_t last_activity_ns;
    uint64_t timeout_ms;
    uint8_t  session_key[MEMSHADOW_SESSION_KEY_SIZE];
    uint8_t  hmac_key[MEMSHADOW_SESSION_KEY_SIZE];
    uint64_t key_rotation_count;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} memshadow_session_t;

typedef struct {
    uint32_t active;
    uint32_t idle;
    uint32_t expired;
    uint32_t closing;
    uint64_t total_messages_sent;
    uint64_t total_messages_received;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
} memshadow_session_stats_t;

typedef struct {
    memshadow_session_t sessions[MEMSHADOW_SESSION_MAX];
    uint32_t session_count;
    uint64_t default_timeout_ms;
    uint64_t key_rotation_interval_ms;
} memshadow_session_manager_t;

void memshadow_session_manager_init(memshadow_session_manager_t *mgr, uint64_t timeout_ms);

int memshadow_session_create(memshadow_session_manager_t *mgr,
                             const char *peer_id,
                             const uint8_t *session_key,
                             const uint8_t *hmac_key,
                             char *out_session_id);

memshadow_session_t *memshadow_session_find(memshadow_session_manager_t *mgr,
                                             const char *session_id);

memshadow_session_t *memshadow_session_find_by_peer(memshadow_session_manager_t *mgr,
                                                     const char *peer_id);

int memshadow_session_close(memshadow_session_manager_t *mgr, const char *session_id);

void memshadow_session_record_send(memshadow_session_t *session, uint64_t bytes);

void memshadow_session_record_receive(memshadow_session_t *session, uint64_t bytes);

int memshadow_session_rotate_key(memshadow_session_t *session,
                                  const uint8_t *new_key,
                                  const uint8_t *new_hmac);

void memshadow_session_tick(memshadow_session_manager_t *mgr);

void memshadow_session_cleanup_expired(memshadow_session_manager_t *mgr);

void memshadow_session_get_stats(const memshadow_session_manager_t *mgr,
                                  memshadow_session_stats_t *stats);

uint32_t memshadow_session_active_count(const memshadow_session_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_SESSION_H */
