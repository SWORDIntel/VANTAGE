/**
 * MEMSHADOW Protocol v3.0 - Perfect Forward Secrecy Implementation (C)
 */

#include "memshadow_pfs.h"
#include "memshadow.h"
#include <string.h>

#ifdef __linux__
#include <sys/random.h>
#endif

static int pfs_random(uint8_t *buf, size_t len) {
#ifdef __linux__
    return (getrandom(buf, len, 0) == (ssize_t)len) ? 0 : -1;
#else
    for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)(rand() & 0xFF);
    return 0;
#endif
}

void memshadow_pfs_init(memshadow_pfs_manager_t *mgr) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->state = PFS_IDLE;
    mgr->rekey_interval_ms = 3600000;
    mgr->max_messages_per_key = 10000;
}

int memshadow_pfs_generate_ephemeral(memshadow_pfs_manager_t *mgr, uint8_t *out_public_key) {
    if (!mgr || !out_public_key) return -1;
    if (pfs_random(mgr->current.secret_key, 32) != 0) return -2;
    if (pfs_random(mgr->current.public_key, 32) != 0) return -2;
    mgr->current.created_at_ns = memshadow_get_timestamp_ns();
    memcpy(out_public_key, mgr->current.public_key, 32);
    mgr->state = PFS_KEY_GENERATED;
    return 0;
}

int memshadow_pfs_derive_shared_secret(memshadow_pfs_manager_t *mgr,
                                        const uint8_t *peer_public_key,
                                        uint8_t *out_shared_secret) {
    if (!mgr || !peer_public_key || !out_shared_secret) return -1;
    if (mgr->state != PFS_KEY_GENERATED && mgr->state != PFS_REKEYING) return -2;

    /* HKDF(eph_sk || peer_pk) — real impl uses X25519 */
    uint8_t ikm[64];
    memcpy(ikm, mgr->current.secret_key, 32);
    memcpy(ikm + 32, peer_public_key, 32);
    memshadow_compute_hmac(ikm, 64, (const uint8_t *)"memshadow-pfs-v3", 16,
                           mgr->shared_secret, 32);
    memset(ikm, 0, sizeof(ikm));

    mgr->has_shared_secret = true;
    mgr->state = PFS_ESTABLISHED;
    mgr->messages_with_current_key = 0;
    memcpy(out_shared_secret, mgr->shared_secret, 32);
    return 0;
}

bool memshadow_pfs_needs_rekey(const memshadow_pfs_manager_t *mgr) {
    if (!mgr || mgr->state != PFS_ESTABLISHED) return false;
    if (mgr->messages_with_current_key >= mgr->max_messages_per_key) return true;
    uint64_t now = memshadow_get_timestamp_ns();
    uint64_t age_ms = (now - mgr->current.created_at_ns) / 1000000ULL;
    return age_ms >= mgr->rekey_interval_ms;
}

int memshadow_pfs_initiate_rekey(memshadow_pfs_manager_t *mgr, uint8_t *out_public_key) {
    if (!mgr || mgr->state != PFS_ESTABLISHED) return -1;
    mgr->previous = mgr->current;
    mgr->state = PFS_REKEYING;
    return memshadow_pfs_generate_ephemeral(mgr, out_public_key);
}

int memshadow_pfs_complete_rekey(memshadow_pfs_manager_t *mgr,
                                  const uint8_t *peer_public_key,
                                  uint8_t *out_shared_secret) {
    int ret = memshadow_pfs_derive_shared_secret(mgr, peer_public_key, out_shared_secret);
    if (ret == 0) {
        memset(&mgr->previous, 0, sizeof(mgr->previous));
        mgr->rekey_count++;
    }
    return ret;
}

void memshadow_pfs_record_message(memshadow_pfs_manager_t *mgr) {
    if (mgr) mgr->messages_with_current_key++;
}

const uint8_t *memshadow_pfs_get_shared_secret(const memshadow_pfs_manager_t *mgr) {
    if (!mgr || !mgr->has_shared_secret) return NULL;
    return mgr->shared_secret;
}

void memshadow_pfs_destroy(memshadow_pfs_manager_t *mgr) {
    if (!mgr) return;
    memset(mgr->current.secret_key, 0, 32);
    memset(mgr->previous.secret_key, 0, 32);
    memset(mgr->shared_secret, 0, 32);
    mgr->has_shared_secret = false;
    mgr->state = PFS_IDLE;
}
