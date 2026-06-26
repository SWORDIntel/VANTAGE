/**
 * MEMSHADOW Protocol v3.0 - Perfect Forward Secrecy (C)
 *
 * Ephemeral key exchange for forward secrecy.
 */

#ifndef MEMSHADOW_PFS_H
#define MEMSHADOW_PFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PFS_IDLE = 0,
    PFS_KEY_GENERATED = 1,
    PFS_ESTABLISHED = 2,
    PFS_REKEYING = 3,
} memshadow_pfs_state_t;

typedef struct {
    uint8_t  public_key[32];
    uint8_t  secret_key[32];
    uint64_t created_at_ns;
} memshadow_ephemeral_keypair_t;

typedef struct {
    memshadow_pfs_state_t state;
    memshadow_ephemeral_keypair_t current;
    memshadow_ephemeral_keypair_t previous;
    uint8_t  shared_secret[32];
    bool     has_shared_secret;
    uint64_t rekey_interval_ms;
    uint64_t rekey_count;
    uint64_t max_messages_per_key;
    uint64_t messages_with_current_key;
} memshadow_pfs_manager_t;

void memshadow_pfs_init(memshadow_pfs_manager_t *mgr);
int memshadow_pfs_generate_ephemeral(memshadow_pfs_manager_t *mgr, uint8_t *out_public_key);
int memshadow_pfs_derive_shared_secret(memshadow_pfs_manager_t *mgr, const uint8_t *peer_public_key, uint8_t *out_shared_secret);
bool memshadow_pfs_needs_rekey(const memshadow_pfs_manager_t *mgr);
int memshadow_pfs_initiate_rekey(memshadow_pfs_manager_t *mgr, uint8_t *out_public_key);
int memshadow_pfs_complete_rekey(memshadow_pfs_manager_t *mgr, const uint8_t *peer_public_key, uint8_t *out_shared_secret);
void memshadow_pfs_record_message(memshadow_pfs_manager_t *mgr);
const uint8_t *memshadow_pfs_get_shared_secret(const memshadow_pfs_manager_t *mgr);
void memshadow_pfs_destroy(memshadow_pfs_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_PFS_H */
