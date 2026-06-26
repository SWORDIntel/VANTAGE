/**
 * MEMSHADOW Hub Fingerprint Verification - C Header
 * 
 * CSNA 2.0 compliant hub identity fingerprint verification.
 * Provides irrevocable binding to hub identity through SHA-384 fingerprint.
 * 
 * Once a hub is registered/detected, its fingerprint is bound irrevocably
 * to prevent impersonation attacks.
 */

#ifndef MEMSHADOW_HUB_FINGERPRINT_H
#define MEMSHADOW_HUB_FINGERPRINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SHA-384 fingerprint size (48 bytes = 96 hex characters) */
#define MEMSHADOW_HUB_FINGERPRINT_SIZE 48
#define MEMSHADOW_HUB_FINGERPRINT_HEX_SIZE 96

/* Hub Fingerprint Structure */
typedef struct {
    char hub_node_id[256];           /* Hub node identifier */
    uint8_t fingerprint[MEMSHADOW_HUB_FINGERPRINT_SIZE];  /* SHA-384 fingerprint */
    uint8_t public_key[512];          /* Public key (variable size, max 512 bytes) */
    size_t public_key_len;            /* Actual public key length */
    int64_t first_seen_timestamp;     /* Unix timestamp (seconds) */
    int64_t last_verified_timestamp;  /* Unix timestamp (seconds) */
    uint32_t verification_count;      /* Number of successful verifications */
} memshadow_hub_fingerprint_t;

/* Hub Fingerprint Manager */
typedef struct memshadow_hub_fingerprint_manager memshadow_hub_fingerprint_manager_t;

/**
 * Create a new hub fingerprint manager.
 * 
 * @param storage_path Path to fingerprint storage file (NULL = default: ~/.dsmil/hub_fingerprints.json)
 * @return Manager instance or NULL on error
 */
memshadow_hub_fingerprint_manager_t *memshadow_hub_fingerprint_manager_create(const char *storage_path);

/**
 * Destroy fingerprint manager and free resources.
 * 
 * @param manager Manager instance
 */
void memshadow_hub_fingerprint_manager_destroy(memshadow_hub_fingerprint_manager_t *manager);

/**
 * Compute SHA-384 fingerprint of public key (CSNA 2.0 compliant).
 * 
 * @param public_key Public key bytes
 * @param public_key_len Length of public key
 * @param fingerprint Output buffer (must be at least MEMSHADOW_HUB_FINGERPRINT_SIZE bytes)
 * @return 0 on success, -1 on error
 */
int memshadow_hub_fingerprint_compute(const uint8_t *public_key, size_t public_key_len,
                                      uint8_t *fingerprint);

/**
 * Compute SHA-384 fingerprint and return as hex string.
 * 
 * @param public_key Public key bytes
 * @param public_key_len Length of public key
 * @param hex_output Output buffer (must be at least MEMSHADOW_HUB_FINGERPRINT_HEX_SIZE + 1 bytes)
 * @return 0 on success, -1 on error
 */
int memshadow_hub_fingerprint_compute_hex(const uint8_t *public_key, size_t public_key_len,
                                          char *hex_output);

/**
 * Register or verify hub fingerprint.
 * 
 * First call registers the hub fingerprint irrevocably.
 * Subsequent calls verify the fingerprint matches.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub node identifier
 * @param public_key Public key bytes
 * @param public_key_len Length of public key
 * @param error_msg Output buffer for error message (NULL if not needed)
 * @param error_msg_size Size of error message buffer
 * @return 0 if valid/registered, -1 if fingerprint mismatch (impersonation detected)
 */
int memshadow_hub_fingerprint_register(memshadow_hub_fingerprint_manager_t *manager,
                                        const char *hub_node_id,
                                        const uint8_t *public_key,
                                        size_t public_key_len,
                                        char *error_msg,
                                        size_t error_msg_size);

/**
 * Verify hub fingerprint matches stored value.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub node identifier
 * @param public_key Public key bytes
 * @param public_key_len Length of public key
 * @param error_msg Output buffer for error message (NULL if not needed)
 * @param error_msg_size Size of error message buffer
 * @return 0 if fingerprint matches, -1 if mismatch or not registered
 */
int memshadow_hub_fingerprint_verify(memshadow_hub_fingerprint_manager_t *manager,
                                     const char *hub_node_id,
                                     const uint8_t *public_key,
                                     size_t public_key_len,
                                     char *error_msg,
                                     size_t error_msg_size);

/**
 * Check if hub is registered.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub node identifier
 * @return true if registered, false otherwise
 */
bool memshadow_hub_fingerprint_is_registered(memshadow_hub_fingerprint_manager_t *manager,
                                              const char *hub_node_id);

/**
 * Get stored fingerprint for a hub.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub node identifier
 * @param fingerprint Output structure (must be allocated by caller)
 * @return 0 on success, -1 if hub not found
 */
int memshadow_hub_fingerprint_get(memshadow_hub_fingerprint_manager_t *manager,
                                  const char *hub_node_id,
                                  memshadow_hub_fingerprint_t *fingerprint);

/**
 * Register hub's own identity (hub self-registration).
 * 
 * Called by a hub node to register its own public key fingerprint.
 * This creates an irrevocable binding to the hub's identity.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub's own node identifier
 * @param public_key Hub's own public key
 * @param public_key_len Length of public key
 * @param error_msg Output buffer for error message (NULL if not needed)
 * @param error_msg_size Size of error message buffer
 * @return 0 on success, -1 on error
 */
int memshadow_hub_fingerprint_register_self(memshadow_hub_fingerprint_manager_t *manager,
                                             const char *hub_node_id,
                                             const uint8_t *public_key,
                                             size_t public_key_len,
                                             char *error_msg,
                                             size_t error_msg_size);

/**
 * Clear a hub fingerprint (for testing/recovery only).
 * 
 * WARNING: This should only be used for testing or recovery scenarios.
 * In production, fingerprints should be irrevocable.
 * 
 * @param manager Manager instance
 * @param hub_node_id Hub node identifier to clear
 * @return 0 on success, -1 if hub not found
 */
int memshadow_hub_fingerprint_clear(memshadow_hub_fingerprint_manager_t *manager,
                                    const char *hub_node_id);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_HUB_FINGERPRINT_H */
