/**
 * MEMSHADOW TLS Extension Implementation (C)
 * 
 * APT41-style TLS extensions for carrying extended flags.
 */

#include "memshadow_tls.h"
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

/* Use standalone HMAC implementation */
#include <string.h>
#include <stdint.h>
#include "memshadow.h"
#define USE_OPENSSL_HMAC 0

#define TLS_EXTENSION_HEADER_SIZE 4  /* type(2) + length(2) */
#define TLS_EXTENSION_TOTAL_SIZE (TLS_EXTENSION_HEADER_SIZE + MEMSHADOW_TLS_EXTENSION_DATA_SIZE)

int memshadow_tls_init(memshadow_tls_handler_t *handler, const uint8_t *key) {
    if (!handler) {
        return -1;
    }
    
    if (key) {
        memcpy(handler->extension_key, key, 32);
    } else {
        /* Generate random key */
        for (int i = 0; i < 32; i++) {
            handler->extension_key[i] = (uint8_t)(time(NULL) + i);
        }
    }
    
    return 0;
}

int memshadow_tls_create_client_extension(
    const memshadow_tls_handler_t *handler,
    uint32_t extended_flags,
    uint16_t protocol_version,
    uint8_t *extension_out,
    size_t *extension_len,
    size_t max_len
) {
    if (!handler || !extension_out || !extension_len || max_len < TLS_EXTENSION_TOTAL_SIZE) {
        return -1;
    }
    
    uint32_t timestamp = (uint32_t)time(NULL);
    
    /* Build extension data */
    uint8_t extension_data[MEMSHADOW_TLS_EXTENSION_DATA_SIZE];
    size_t offset = 0;
    
    /* Protocol version (2 bytes, big-endian) */
    uint16_t version_be = htons(protocol_version);
    memcpy(extension_data + offset, &version_be, 2);
    offset += 2;
    
    /* Extended flags (4 bytes, big-endian) */
    uint32_t flags_be = htonl(extended_flags);
    memcpy(extension_data + offset, &flags_be, 4);
    offset += 4;
    
    /* Timestamp (4 bytes, big-endian) */
    uint32_t timestamp_be = htonl(timestamp);
    memcpy(extension_data + offset, &timestamp_be, 4);
    offset += 4;
    
    /* Calculate HMAC-SHA384 over version + flags + timestamp */
    uint8_t hmac[48];
    if (memshadow_compute_hmac(extension_data, 10,
                               handler->extension_key, 32,
                               hmac, sizeof(hmac)) != 0) {
        return -1;
    }
    memcpy(extension_data + offset, hmac, 48);
    
    /* Build TLS extension: type(2) + length(2) + data */
    offset = 0;
    
    /* Extension type (2 bytes, big-endian) */
    uint16_t ext_type_be = htons(MEMSHADOW_TLS_EXTENSION_ID);
    memcpy(extension_out + offset, &ext_type_be, 2);
    offset += 2;
    
    /* Extension length (2 bytes, big-endian) */
    uint16_t ext_len_be = htons(MEMSHADOW_TLS_EXTENSION_DATA_SIZE);
    memcpy(extension_out + offset, &ext_len_be, 2);
    offset += 2;
    
    /* Extension data */
    memcpy(extension_out + offset, extension_data, MEMSHADOW_TLS_EXTENSION_DATA_SIZE);
    
    *extension_len = TLS_EXTENSION_TOTAL_SIZE;
    return 0;
}

int memshadow_tls_create_server_extension(
    const memshadow_tls_handler_t *handler,
    uint32_t negotiated_flags,
    uint16_t protocol_version,
    uint8_t *extension_out,
    size_t *extension_len,
    size_t max_len
) {
    /* Same as client extension */
    return memshadow_tls_create_client_extension(
        handler, negotiated_flags, protocol_version,
        extension_out, extension_len, max_len
    );
}

int memshadow_tls_parse_extension(
    const memshadow_tls_handler_t *handler,
    const uint8_t *extension_data,
    size_t extension_data_len,
    memshadow_tls_extension_data_t *parsed
) {
    if (!handler || !extension_data || !parsed ||
        extension_data_len < MEMSHADOW_TLS_EXTENSION_DATA_SIZE) {
        return -1;
    }
    
    size_t offset = 0;
    
    /* Parse protocol version */
    uint16_t version_be;
    memcpy(&version_be, extension_data + offset, 2);
    parsed->protocol_version = ntohs(version_be);
    offset += 2;
    
    /* Parse extended flags */
    uint32_t flags_be;
    memcpy(&flags_be, extension_data + offset, 4);
    parsed->extended_flags = ntohl(flags_be);
    offset += 4;
    
    /* Parse timestamp */
    uint32_t timestamp_be;
    memcpy(&timestamp_be, extension_data + offset, 4);
    parsed->timestamp = ntohl(timestamp_be);
    offset += 4;
    
    /* Get received HMAC */
    memcpy(parsed->hmac, extension_data + offset, 48);

    /* Verify HMAC */
    uint8_t expected_hmac[48];
    if (memshadow_compute_hmac(extension_data, 10,
                               handler->extension_key, 32,
                               expected_hmac, sizeof(expected_hmac)) != 0) {
        return -1;
    }
    if (memcmp(parsed->hmac, expected_hmac, 32) != 0) {
        return -1;  /* HMAC validation failed */
    }
    
    return 0;
}

uint32_t memshadow_tls_negotiate_flags(uint32_t client_flags, uint32_t server_flags) {
    /* Return intersection (flags supported by both) */
    return client_flags & server_flags;
}

void memshadow_tls_context_init(memshadow_tls_context_t *ctx) {
    if (!ctx) return;
    
    memset(ctx, 0, sizeof(memshadow_tls_context_t));
}

void memshadow_tls_context_set_flags(
    memshadow_tls_context_t *ctx,
    uint32_t flags,
    uint16_t protocol_version
) {
    if (!ctx) return;
    
    ctx->extended_flags = flags;
    ctx->protocol_version = protocol_version;
    ctx->timestamp = (uint32_t)time(NULL);
    ctx->negotiated = true;
}

uint32_t memshadow_tls_context_get_all_flags(
    const memshadow_tls_context_t *ctx,
    uint8_t base_flags
) {
    if (!ctx || !ctx->negotiated) {
        return base_flags;
    }
    
    /* Base flags in low 8 bits, extended flags in high bits */
    return base_flags | (ctx->extended_flags << 8);
}

bool memshadow_tls_context_has_flag(
    const memshadow_tls_context_t *ctx,
    uint32_t flag
) {
    if (!ctx || !ctx->negotiated) {
        return false;
    }
    
    return (ctx->extended_flags & flag) != 0;
}
