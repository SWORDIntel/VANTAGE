/**
 * MEMSHADOW TLS Extension Support
 * 
 * APT41-style TLS extensions for carrying extended flags.
 * Extended flags are negotiated once per TLS session during handshake.
 */

#ifndef MEMSHADOW_TLS_H
#define MEMSHADOW_TLS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TLS Extension ID for MEMSHADOW */
#define MEMSHADOW_TLS_EXTENSION_ID 0xFC01

/* Extension data size (without type/length header) */
#define MEMSHADOW_TLS_EXTENSION_DATA_SIZE 42

/* Extension structure */
typedef struct {
    uint16_t protocol_version;  /* Protocol version (0x0300 = v3.0) */
    uint32_t extended_flags;     /* Extended flags (32-bit) */
    uint32_t timestamp;          /* Unix timestamp */
    uint8_t hmac[48];            /* HMAC-SHA384 */
} memshadow_tls_extension_data_t;

/* TLS Extension Handler */
typedef struct {
    uint8_t extension_key[32];    /* HMAC key for extension validation */
} memshadow_tls_handler_t;

/* Connection Context */
typedef struct {
    uint32_t extended_flags;     /* Negotiated extended flags */
    uint16_t protocol_version;   /* Protocol version */
    uint32_t timestamp;          /* Negotiation timestamp */
    bool negotiated;             /* Whether flags have been negotiated */
} memshadow_tls_context_t;

/* Initialize TLS extension handler */
int memshadow_tls_init(memshadow_tls_handler_t *handler, const uint8_t *key);

/* Create ClientHello extension */
int memshadow_tls_create_client_extension(
    const memshadow_tls_handler_t *handler,
    uint32_t extended_flags,
    uint16_t protocol_version,
    uint8_t *extension_out,
    size_t *extension_len,
    size_t max_len
);

/* Create ServerHello extension */
int memshadow_tls_create_server_extension(
    const memshadow_tls_handler_t *handler,
    uint32_t negotiated_flags,
    uint16_t protocol_version,
    uint8_t *extension_out,
    size_t *extension_len,
    size_t max_len
);

/* Parse TLS extension data */
int memshadow_tls_parse_extension(
    const memshadow_tls_handler_t *handler,
    const uint8_t *extension_data,
    size_t extension_data_len,
    memshadow_tls_extension_data_t *parsed
);

/* Negotiate flags (intersection of client and server flags) */
uint32_t memshadow_tls_negotiate_flags(uint32_t client_flags, uint32_t server_flags);

/* Initialize connection context */
void memshadow_tls_context_init(memshadow_tls_context_t *ctx);

/* Set negotiated flags in context */
void memshadow_tls_context_set_flags(
    memshadow_tls_context_t *ctx,
    uint32_t flags,
    uint16_t protocol_version
);

/* Get all flags (base + extended) */
uint32_t memshadow_tls_context_get_all_flags(
    const memshadow_tls_context_t *ctx,
    uint8_t base_flags
);

/* Check if extended flag is set */
bool memshadow_tls_context_has_flag(
    const memshadow_tls_context_t *ctx,
    uint32_t flag
);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_TLS_H */
