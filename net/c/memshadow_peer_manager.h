/**
 * MEMSHADOW Protocol v3.0 - Peer Manager (C)
 *
 * Large local network peer management with health monitoring,
 * load balancing, and automatic failover.
 */

#ifndef MEMSHADOW_PEER_MANAGER_H
#define MEMSHADOW_PEER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMSHADOW_PEER_MAX 256

typedef enum {
    PEER_CONNECTED = 0,
    PEER_CONNECTING = 1,
    PEER_DISCONNECTED = 2,
    PEER_BANNED = 3,
    PEER_UNREACHABLE = 4,
} memshadow_peer_status_t;

typedef enum {
    SELECT_BEST_HEALTH = 0,
    SELECT_ROUND_ROBIN = 1,
    SELECT_RANDOM = 2,
    SELECT_LOWEST_LATENCY = 3,
    SELECT_HIGHEST_BANDWIDTH = 4,
} memshadow_selection_strategy_t;

typedef struct {
    double   latency_ms;
    double   packet_loss;
    uint64_t bandwidth_bps;
    uint64_t last_ping_ns;
    uint32_t ping_failures;
    uint32_t max_ping_failures;
} memshadow_peer_health_t;

typedef struct {
    char     node_id[64];
    uint32_t address_ipv4;
    uint16_t port;
    memshadow_peer_status_t status;
    memshadow_peer_health_t health;
    uint32_t capabilities;
    uint16_t version;
    uint64_t connected_since_ns;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} memshadow_peer_info_t;

typedef struct {
    memshadow_peer_info_t peers[MEMSHADOW_PEER_MAX];
    uint16_t peer_count;
    uint16_t max_peers;
    uint32_t ping_interval_ms;
    uint64_t ban_duration_ms;
    memshadow_selection_strategy_t strategy;
    uint16_t round_robin_index;
} memshadow_peer_manager_t;

void memshadow_peer_manager_init(memshadow_peer_manager_t *mgr, uint16_t max_peers);
int memshadow_peer_add(memshadow_peer_manager_t *mgr, const memshadow_peer_info_t *peer);
int memshadow_peer_remove(memshadow_peer_manager_t *mgr, const char *node_id);
memshadow_peer_info_t *memshadow_peer_find(memshadow_peer_manager_t *mgr, const char *node_id);
memshadow_peer_info_t *memshadow_peer_select(memshadow_peer_manager_t *mgr);
void memshadow_peer_connect(memshadow_peer_manager_t *mgr, const char *node_id);
void memshadow_peer_disconnect(memshadow_peer_manager_t *mgr, const char *node_id);
void memshadow_peer_ban(memshadow_peer_manager_t *mgr, const char *node_id);
void memshadow_peer_record_ping(memshadow_peer_info_t *peer, double latency_ms);
void memshadow_peer_record_ping_failure(memshadow_peer_info_t *peer);
double memshadow_peer_health_score(const memshadow_peer_health_t *health);
bool memshadow_peer_is_healthy(const memshadow_peer_health_t *health);
void memshadow_peer_tick(memshadow_peer_manager_t *mgr);
uint16_t memshadow_peer_connected_count(const memshadow_peer_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_PEER_MANAGER_H */
