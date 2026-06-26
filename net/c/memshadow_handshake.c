/**
 * MEMSHADOW Protocol v3.0 - Handshake Implementation (C)
 */

#include "memshadow_handshake.h"
#include "memshadow.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef __linux__
#include <sys/random.h>
#endif

static int secure_random(uint8_t *buf, size_t len) {
#ifdef __linux__
    return (getrandom(buf, len, 0) == (ssize_t)len) ? 0 : -1;
#else
    for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)(rand() & 0xFF);
    return 0;
#endif
}

void memshadow_handshake_init(memshadow_handshake_ctx_t *ctx, uint32_t capabilities) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = HANDSHAKE_IDLE;
    ctx->local_capabilities = capabilities;
    ctx->local_version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    ctx->timeout_ms = 30000;
    ctx->has_session_key = false;
}

int memshadow_handshake_create_hello(
    memshadow_handshake_ctx_t *ctx,
    const char *node_id,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *out_size
) {
    if (ctx->state != HANDSHAKE_IDLE) return -1;

    size_t node_id_len = strlen(node_id);
    if (node_id_len > 63) node_id_len = 63;
    size_t required = 1 + 2 + 4 + 2 + node_id_len + 8;
    if (buffer_size < required) return -2;

    uint64_t timestamp = memshadow_get_timestamp_ns();
    size_t offset = 0;

    buffer[offset++] = HANDSHAKE_STEP_HELLO;

    /* Version (2 bytes, big-endian) */
    buffer[offset++] = (ctx->local_version >> 8) & 0xFF;
    buffer[offset++] = ctx->local_version & 0xFF;

    /* Capabilities (4 bytes, big-endian) */
    buffer[offset++] = (ctx->local_capabilities >> 24) & 0xFF;
    buffer[offset++] = (ctx->local_capabilities >> 16) & 0xFF;
    buffer[offset++] = (ctx->local_capabilities >> 8) & 0xFF;
    buffer[offset++] = ctx->local_capabilities & 0xFF;

    /* Node ID length (2 bytes) + Node ID */
    buffer[offset++] = (node_id_len >> 8) & 0xFF;
    buffer[offset++] = node_id_len & 0xFF;
    memcpy(buffer + offset, node_id, node_id_len);
    offset += node_id_len;

    /* Timestamp (8 bytes) */
    for (int i = 7; i >= 0; i--) {
        buffer[offset++] = (timestamp >> (i * 8)) & 0xFF;
    }

    *out_size = offset;
    ctx->state = HANDSHAKE_HELLO_SENT;
    ctx->started_at_ns = timestamp;
    strncpy(ctx->peer_node_id, node_id, sizeof(ctx->peer_node_id) - 1);

    return 0;
}

int memshadow_handshake_process_hello(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *hello_data,
    size_t hello_size,
    uint8_t *ack_buffer,
    size_t ack_buffer_size,
    size_t *ack_size
) {
    if (hello_size < 9 || hello_data[0] != HANDSHAKE_STEP_HELLO) return -1;

    /* Parse peer version */
    ctx->peer_version = ((uint16_t)hello_data[1] << 8) | hello_data[2];
    uint8_t peer_major = hello_data[1];
    uint8_t local_major = (ctx->local_version >> 8) & 0xFF;
    if (peer_major != local_major) return -3; /* Incompatible major version */

    /* Parse peer capabilities */
    ctx->peer_capabilities = ((uint32_t)hello_data[3] << 24) |
                             ((uint32_t)hello_data[4] << 16) |
                             ((uint32_t)hello_data[5] << 8) |
                             (uint32_t)hello_data[6];

    /* Negotiate capabilities (intersection) */
    ctx->negotiated_capabilities = ctx->local_capabilities & ctx->peer_capabilities;

    /* Parse node ID */
    uint16_t node_id_len = ((uint16_t)hello_data[7] << 8) | hello_data[8];
    if (hello_size < (size_t)(9 + node_id_len)) return -2;
    size_t copy_len = (node_id_len < sizeof(ctx->peer_node_id) - 1) ? node_id_len : sizeof(ctx->peer_node_id) - 1;
    memcpy(ctx->peer_node_id, hello_data + 9, copy_len);
    ctx->peer_node_id[copy_len] = '\0';

    /* Generate challenge (32 random bytes) */
    if (secure_random(ctx->challenge, 32) != 0) return -4;

    /* Generate session ID (16 random bytes) */
    if (secure_random(ctx->session_id, 16) != 0) return -4;

    /* Build HELLO_ACK: step(1) + version(2) + neg_caps(4) + challenge(32) + session_id(16) = 55 */
    size_t required = 55;
    if (ack_buffer_size < required) return -2;

    size_t offset = 0;
    ack_buffer[offset++] = HANDSHAKE_STEP_HELLO_ACK;
    ack_buffer[offset++] = (ctx->local_version >> 8) & 0xFF;
    ack_buffer[offset++] = ctx->local_version & 0xFF;
    ack_buffer[offset++] = (ctx->negotiated_capabilities >> 24) & 0xFF;
    ack_buffer[offset++] = (ctx->negotiated_capabilities >> 16) & 0xFF;
    ack_buffer[offset++] = (ctx->negotiated_capabilities >> 8) & 0xFF;
    ack_buffer[offset++] = ctx->negotiated_capabilities & 0xFF;
    memcpy(ack_buffer + offset, ctx->challenge, 32);
    offset += 32;
    memcpy(ack_buffer + offset, ctx->session_id, 16);
    offset += 16;

    *ack_size = offset;
    ctx->state = HANDSHAKE_HELLO_ACK_SENT;
    ctx->started_at_ns = memshadow_get_timestamp_ns();

    return 0;
}

int memshadow_handshake_process_hello_ack(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *ack_data,
    size_t ack_size,
    const uint8_t *hmac_key,
    size_t hmac_key_size,
    uint8_t *response_buffer,
    size_t response_buffer_size,
    size_t *response_size
) {
    if (ctx->state != HANDSHAKE_HELLO_SENT) return -1;
    if (ack_size < 55 || ack_data[0] != HANDSHAKE_STEP_HELLO_ACK) return -2;

    /* Parse negotiated capabilities */
    ctx->negotiated_capabilities = ((uint32_t)ack_data[3] << 24) |
                                   ((uint32_t)ack_data[4] << 16) |
                                   ((uint32_t)ack_data[5] << 8) |
                                   (uint32_t)ack_data[6];

    /* Extract challenge and session ID */
    memcpy(ctx->challenge, ack_data + 7, 32);
    memcpy(ctx->session_id, ack_data + 39, 16);

    /* Compute HMAC-SHA256(challenge, hmac_key) */
    uint8_t hmac_output[32];
    if (memshadow_compute_hmac(ctx->challenge, 32, hmac_key, hmac_key_size, hmac_output, 32) != 0) {
        return -4;
    }

    /* Build CHALLENGE_RESPONSE: step(1) + session_id(16) + hmac(32) = 49 */
    size_t required = 49;
    if (response_buffer_size < required) return -2;

    size_t offset = 0;
    response_buffer[offset++] = HANDSHAKE_STEP_CHALLENGE_RESPONSE;
    memcpy(response_buffer + offset, ctx->session_id, 16);
    offset += 16;
    memcpy(response_buffer + offset, hmac_output, 32);
    offset += 32;

    *response_size = offset;
    ctx->state = HANDSHAKE_CHALLENGE_RESPONSE_SENT;

    return 0;
}

int memshadow_handshake_process_challenge_response(
    memshadow_handshake_ctx_t *ctx,
    const uint8_t *response_data,
    size_t response_size,
    const uint8_t *hmac_key,
    size_t hmac_key_size,
    uint8_t *established_buffer,
    size_t established_buffer_size,
    size_t *established_size
) {
    if (ctx->state != HANDSHAKE_HELLO_ACK_SENT) return -1;
    if (response_size < 49 || response_data[0] != HANDSHAKE_STEP_CHALLENGE_RESPONSE) return -2;

    /* Verify session ID */
    if (memcmp(response_data + 1, ctx->session_id, 16) != 0) return -3;

    /* Verify HMAC response */
    const uint8_t *received_hmac = response_data + 17;
    if (memshadow_verify_hmac(ctx->challenge, 32, hmac_key, hmac_key_size, received_hmac, 32) != 0) {
        ctx->state = HANDSHAKE_FAILED;
        return -5; /* Authentication failed */
    }

    /* Generate session key */
    if (secure_random(ctx->session_key, 32) != 0) return -4;
    ctx->has_session_key = true;

    /* Build SESSION_ESTABLISHED: step(1) + session_id(16) + key_mac(32) = 49 */
    size_t required = 49;
    if (established_buffer_size < required) return -2;

    uint8_t key_mac[32];
    memshadow_compute_hmac(ctx->session_key, 32, hmac_key, hmac_key_size, key_mac, 32);

    size_t offset = 0;
    established_buffer[offset++] = HANDSHAKE_STEP_SESSION_ESTABLISHED;
    memcpy(established_buffer + offset, ctx->session_id, 16);
    offset += 16;
    memcpy(established_buffer + offset, key_mac, 32);
    offset += 32;

    *established_size = offset;
    ctx->state = HANDSHAKE_ESTABLISHED;

    return 0;
}

bool memshadow_handshake_is_timed_out(const memshadow_handshake_ctx_t *ctx) {
    if (ctx->started_at_ns == 0) return false;
    uint64_t now = memshadow_get_timestamp_ns();
    uint64_t elapsed_ms = (now - ctx->started_at_ns) / 1000000;
    return elapsed_ms > ctx->timeout_ms;
}

void memshadow_handshake_reset(memshadow_handshake_ctx_t *ctx) {
    uint32_t caps = ctx->local_capabilities;
    uint16_t ver = ctx->local_version;
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = HANDSHAKE_IDLE;
    ctx->local_capabilities = caps;
    ctx->local_version = ver;
    ctx->timeout_ms = 30000;
}

uint32_t memshadow_handshake_get_negotiated_caps(const memshadow_handshake_ctx_t *ctx) {
    return ctx->negotiated_capabilities;
}

const uint8_t *memshadow_handshake_get_session_key(const memshadow_handshake_ctx_t *ctx) {
    if (ctx->state == HANDSHAKE_ESTABLISHED && ctx->has_session_key) {
        return ctx->session_key;
    }
    return NULL;
}
