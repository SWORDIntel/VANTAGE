/**
 * MEMSHADOW Protocol - VLAN Topology Discovery and Relay Header
 *
 * Implements VLAN-aware topology discovery and multi-hop relay functionality.
 * Uses breadth-first search (BFS) for finding optimal relay paths.
 * Supports Internet/VLAN-only connectivity classification.
 */

#ifndef MEMSHADOW_VLAN_TOPOLOGY_H
#define MEMSHADOW_VLAN_TOPOLOGY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Node Connectivity Types */
typedef enum {
    VLAN_NODE_CONNECTIVITY_UNKNOWN = 0,
    VLAN_NODE_CONNECTIVITY_VLAN_ONLY = 1,
    VLAN_NODE_CONNECTIVITY_INTERNET = 2,
    VLAN_NODE_CONNECTIVITY_RELAY_CAPABLE = 3,
} memshadow_vlan_connectivity_t;

/* VLAN Node Structure */
typedef struct {
    char *node_id;
    int32_t vlan_id;
    char *address;
    uint16_t port;
    memshadow_vlan_connectivity_t connectivity;
    uint64_t last_seen;
    int32_t relay_capacity;
    int32_t relay_load;
} memshadow_vlan_node_t;

/* Relay Path Structure */
typedef struct {
    char *source;
    char *destination;
    char **hops;           /* Array of node IDs */
    size_t hop_count;
    size_t total_hops;
    uint64_t total_latency;
    size_t vlan_hops;
} memshadow_vlan_relay_path_t;

/* VLAN Relay Manager Structure */
typedef struct {
    char *node_id;
    int32_t vlan_id;
    memshadow_vlan_node_t *nodes;
    size_t node_count;
    size_t node_capacity;
    char ***connections;   /* Array of string arrays */
    size_t *connection_counts;
    size_t max_path_length;
} memshadow_vlan_relay_manager_t;

/* Topology Summary Structure */
typedef struct {
    size_t total_nodes;
    size_t vlan_count;
    size_t internet_nodes;
    size_t relay_nodes;
    size_t active_relays;
} memshadow_vlan_topology_summary_t;

/* Function Declarations */

/* Initialize VLAN node */
int memshadow_vlan_node_init(
    memshadow_vlan_node_t *node,
    const char *node_id,
    int32_t vlan_id,
    const char *address,
    uint16_t port,
    memshadow_vlan_connectivity_t connectivity
);

/* Cleanup VLAN node */
void memshadow_vlan_node_cleanup(memshadow_vlan_node_t *node);

/* Initialize VLAN relay manager */
int memshadow_vlan_relay_manager_init(
    memshadow_vlan_relay_manager_t **manager,
    const char *node_id,
    int32_t vlan_id
);

/* Cleanup VLAN relay manager */
void memshadow_vlan_relay_manager_cleanup(memshadow_vlan_relay_manager_t *manager);

/* Register a node in the topology */
int memshadow_vlan_relay_manager_register_node(
    memshadow_vlan_relay_manager_t *manager,
    memshadow_vlan_node_t node
);

/* Register a connection between nodes */
int memshadow_vlan_relay_manager_register_connection(
    memshadow_vlan_relay_manager_t *manager,
    const char *from_node,
    const char *to_node
);

/* Get topology summary */
void memshadow_vlan_relay_manager_get_topology_summary(
    const memshadow_vlan_relay_manager_t *manager,
    memshadow_vlan_topology_summary_t *summary
);

/* Find relay path using BFS */
int memshadow_vlan_relay_manager_find_relay_path(
    memshadow_vlan_relay_manager_t *manager,
    const char *destination,
    bool prefer_vlan,
    memshadow_vlan_relay_path_t **path
);

/* Initialize relay path */
int memshadow_vlan_relay_path_init(
    memshadow_vlan_relay_path_t **path,
    const char *source,
    const char *destination
);

/* Cleanup relay path */
void memshadow_vlan_relay_path_cleanup(memshadow_vlan_relay_path_t *path);

/* Add hop to relay path */
int memshadow_vlan_relay_path_add_hop(
    memshadow_vlan_relay_path_t *path,
    const char *node_id
);

/* Check connectivity capabilities */
bool memshadow_vlan_connectivity_has_internet_access(
    memshadow_vlan_connectivity_t connectivity
);

bool memshadow_vlan_connectivity_can_relay(
    memshadow_vlan_connectivity_t connectivity
);

/* Update node relay load */
int memshadow_vlan_relay_manager_update_relay_load(
    memshadow_vlan_relay_manager_t *manager,
    const char *node_id,
    int32_t new_load
);

/* Get node relay load */
int32_t memshadow_vlan_relay_manager_get_relay_load(
    const memshadow_vlan_relay_manager_t *manager,
    const char *node_id
);

/* Check if node can accept more relay traffic */
bool memshadow_vlan_node_can_accept_relay(const memshadow_vlan_node_t *node);

/* Update node last seen timestamp */
void memshadow_vlan_node_update_last_seen(memshadow_vlan_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_VLAN_TOPOLOGY_H */