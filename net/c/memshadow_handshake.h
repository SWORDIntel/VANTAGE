/**
 * MEMSHADOW Protocol v3.0 - Handshake (C)
 *
 * Secure handshake with version negotiation, capability exchange,
 * and HMAC-based challenge-response authentication.
 */

#ifndef MEMSHADOW_HANDSHAKE_H
#define MEMSHADOW_HANDSHAKE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Handshake states */
typedef enum {
    HANDSHAKE_IDLE = 0,
    HANDSHAKE_HELLO_SENT = 1,
    HANDSHAKE_HELLO_ACK_SENT = 2,
    HANDSHAKE_CHALLENGE_RESPONSE_SENT = 3,
    HANDSHAKE_ESTABLISHED = 4,
    HANDSHAKE_FAILED = 5,
} memshadow_handshake_state_t;

/* Handshake step sub-types (within MSG_HANDSHAKE) */
typedef enum {
    HANDSHAKE_STEP_HELLO = 0x01,
    HANDSHAKE_STEP_HELLO_ACK = 0x02,
    HANDSHAKE_STEP_CHALLENGE_RESPONSE = 0x03,
    HANDSHAKE_STEP_SESSION_ESTABLISHED = 0x04,
    HANDSHAKE_STEP_SESSION_REJECT = 0x05,
} memshadow_handshake_step_t;

/* Capability bitmask */
#define MEMSHADOW_CAP_PQC              (1 << 0)
#define MEMSHADOW_CAP_VLAN_RELAY       (1 << 1)
#define MEMSHADOW_CAP_DPI_EVASION      (1 << 2)
#define MEMSHADOW_CAP_TRAFFIC_RESIST   (1 << 3)
#define MEMSHADOW_CAP_COVERT_VLAN      (1 << 4)
#define MEMSHADOW_CAP_HUB_FINGERPRINT  (1 << 5)
#define MEMSHADOW_CAP_TLS_EXTENSIONS   (1 << 6)
#define MEMSHADOW_CAP_GOSSIP           (1 << 7)
#define MEMSHADOW_CAP_PFS              (1 << 8)
#define MEMSHADOW_CAP_ZKP              (1 << 9)
#define MEMSHADOW_CAP_TEMPORAL         (1 << 10)
#define MEMSHADOW_CAP_QUANTUM_ENTANGLE (1 << 11)
#define MEMSHADOW_CAP_MORPHIC          (1 << 12)
#define MEMSHADOW_CAP_RDMA             (1 << 13)
#define MEMSHADOW_CAP_DUAL_STREAM      (1 << 14)
#define MEMSHADOW_CAP_ECC              (1 << 15)
#define MEMSHADOW_CAP_ALL              0xFFFF

/* Handshake context */
typedef struct {
    memshadow_handshake_state_t state;
    uint32_t local_capabilities;
    uint32_t peer_capabilities;
    uint32_t negotiated_capabilities;
    uint16_t local_version;
    uint16_t peer_version;
    uint8_t  challenge[32];
    uint8_t  session_id[16];
    uint8_t  session_key[32];
    char     peer_node_id[64];
    uint32_t timeout_ms;
    uint64_t started_at_ns;
    bool     has_session_key;
} memshadow_handshake_ctx_t;

/* Initialize handshake context */
void memshadow_handshake_init(memshadow_handshake_ctx_t *ctx, uint32_t capabilities);

/* Create HELLO message (client → server) */
int memshadow_handshake_create_hello(
    memshadow_handshake_ctx_t *ctx,
    const char *node_id,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *out_size
);

/* Process HELLO and create HELLO_ACK (server side) */
int memshadow_handshake_process_hello(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *hello_data,
    size_t hello_size,
    uint8_t *ack_buffer,
    size_t ack_buffer_size,
    size_t *ack_size
);

/* Process HELLO_ACK and create CHALLENGE_RESPONSE (client side) */
int memshadow_handshake_process_hello_ack(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *ack_data,
    size_t ack_size,
    const uint8_t *hmac_key,
    size_t hmac_key_size,
    uint8_t *response_buffer,
    size_t response_buffer_size,
    size_t *response_size
);

/* Process CHALLENGE_RESPONSE and create SESSION_ESTABLISHED (server side) */
int memshadow_handshake_process_challenge_response(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *response_data,
    size_t response_size,
    const uint8_t *hmac_key,
    size_t hmac_key_size,
    uint8_t *established_buffer,
    size_t established_buffer_size,
    size_t *established_size
);

/* Check if handshake timed out */
bool memshadow_handshake_is_timed_out(const memshadow_handshake_ctx_t *ctx);

/* Reset handshake state */
void memshadow_handshake_reset(memshadow_handshake_ctx_t *ctx);

/* Get negotiated capabilities */
uint32_t memshadow_handshake_get_negotiated_caps(const memshadow_handshake_ctx_t *ctx);

/* Get session key (only valid after ESTABLISHED) */
const uint8_t *memshadow_handshake_get_session_key(const memshadow_handshake_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_HANDSHAKE_H */
