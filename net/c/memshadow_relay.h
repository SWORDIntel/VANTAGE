/**
 * MEMSHADOW Protocol v3.0 - Relay System (C)
 *
 * Multi-hop relay routing with path caching and failover.
 */

#ifndef MEMSHADOW_RELAY_H
#define MEMSHADOW_RELAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMSHADOW_RELAY_MAX_HOPS       10
#define MEMSHADOW_RELAY_MAX_ROUTES     256
#define MEMSHADOW_RELAY_MAX_ALT        3

typedef struct {
    char     node_id[64];
    uint32_t address_ipv4;
    uint16_t port;
    double   latency_ms;
} memshadow_relay_hop_t;

typedef struct {
    memshadow_relay_hop_t hops[MEMSHADOW_RELAY_MAX_HOPS];
    uint8_t  hop_count;
    double   total_latency_ms;
    uint64_t created_at_ns;
    uint64_t ttl_ms;
    uint64_t use_count;
    char     destination[64];
} memshadow_relay_route_t;

typedef struct {
    uint64_t messages_relayed;
    uint64_t bytes_relayed;
    uint64_t routes_discovered;
    uint64_t routes_expired;
    uint64_t relay_failures;
    uint64_t cache_hits;
    uint64_t cache_misses;
} memshadow_relay_stats_t;

typedef struct {
    char     node_id[64];
    memshadow_relay_route_t routes[MEMSHADOW_RELAY_MAX_ROUTES];
    uint16_t route_count;
    uint8_t  max_hops;
    uint64_t route_ttl_ms;
    memshadow_relay_stats_t stats;
} memshadow_relay_manager_t;

void memshadow_relay_init(memshadow_relay_manager_t *mgr, const char *node_id);
int memshadow_relay_add_route(memshadow_relay_manager_t *mgr, const memshadow_relay_route_t *route);
const memshadow_relay_route_t *memshadow_relay_get_route(memshadow_relay_manager_t *mgr, const char *destination);
bool memshadow_relay_has_route(const memshadow_relay_manager_t *mgr, const char *destination);
void memshadow_relay_record_success(memshadow_relay_manager_t *mgr, const char *destination, uint64_t bytes);
void memshadow_relay_record_failure(memshadow_relay_manager_t *mgr, const char *destination);
void memshadow_relay_cleanup(memshadow_relay_manager_t *mgr);
uint16_t memshadow_relay_cached_count(const memshadow_relay_manager_t *mgr);
void memshadow_relay_get_stats(const memshadow_relay_manager_t *mgr, memshadow_relay_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_RELAY_H */
