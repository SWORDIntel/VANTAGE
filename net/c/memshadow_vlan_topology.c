/**
 * MEMSHADOW Protocol - VLAN Topology Discovery and Relay Implementation
 *
 * Implements VLAN-aware topology discovery and multi-hop relay functionality.
 * Uses breadth-first search (BFS) for finding optimal relay paths.
 * Supports Internet/VLAN-only connectivity classification.
 */

#include "memshadow_vlan_topology.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Internal helper functions */
static char *strdup_safe(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

/* VLAN Node Functions */
int memshadow_vlan_node_init(
    memshadow_vlan_node_t *node,
    const char *node_id,
    int32_t vlan_id,
    const char *address,
    uint16_t port,
    memshadow_vlan_connectivity_t connectivity
) {
    if (!node || !node_id || !address) {
        return -1;
    }

    memset(node, 0, sizeof(memshadow_vlan_node_t));

    node->node_id = strdup_safe(node_id);
    node->address = strdup_safe(address);

    if (!node->node_id || !node->address) {
        memshadow_vlan_node_cleanup(node);
        return -1;
    }

    node->vlan_id = vlan_id;
    node->port = port;
    node->connectivity = connectivity;
    node->last_seen = 0; // Will be set by update function
    node->relay_capacity = 10; // Default capacity
    node->relay_load = 0;

    memshadow_vlan_node_update_last_seen(node);

    return 0;
}

void memshadow_vlan_node_cleanup(memshadow_vlan_node_t *node) {
    if (node) {
        free(node->node_id);
        free(node->address);
        memset(node, 0, sizeof(memshadow_vlan_node_t));
    }
}

void memshadow_vlan_node_update_last_seen(memshadow_vlan_node_t *node) {
    if (node) {
        // Simple timestamp - in real implementation, use proper time
        static uint64_t mock_time = 1000000000;
        node->last_seen = mock_time++;
    }
}

bool memshadow_vlan_node_can_accept_relay(const memshadow_vlan_node_t *node) {
    return node &&
           memshadow_vlan_connectivity_can_relay(node->connectivity) &&
           node->relay_load < node->relay_capacity;
}

/* Connectivity Functions */
bool memshadow_vlan_connectivity_has_internet_access(
    memshadow_vlan_connectivity_t connectivity
) {
    return connectivity == VLAN_NODE_CONNECTIVITY_INTERNET ||
           connectivity == VLAN_NODE_CONNECTIVITY_RELAY_CAPABLE;
}

bool memshadow_vlan_connectivity_can_relay(
    memshadow_vlan_connectivity_t connectivity
) {
    return connectivity == VLAN_NODE_CONNECTIVITY_RELAY_CAPABLE;
}

/* Relay Path Functions */
int memshadow_vlan_relay_path_init(
    memshadow_vlan_relay_path_t **path,
    const char *source,
    const char *destination
) {
    if (!path || !source || !destination) {
        return -1;
    }

    *path = calloc(1, sizeof(memshadow_vlan_relay_path_t));
    if (!*path) {
        return -1;
    }

    (*path)->source = strdup_safe(source);
    (*path)->destination = strdup_safe(destination);

    if (!(*path)->source || !(*path)->destination) {
        memshadow_vlan_relay_path_cleanup(*path);
        free(*path);
        *path = NULL;
        return -1;
    }

    // Initialize hops array
    (*path)->hops = malloc(sizeof(char*));
    if (!(*path)->hops) {
        memshadow_vlan_relay_path_cleanup(*path);
        free(*path);
        *path = NULL;
        return -1;
    }

    // Add source as first hop
    (*path)->hops[0] = strdup_safe(source);
    if (!(*path)->hops[0]) {
        memshadow_vlan_relay_path_cleanup(*path);
        free(*path);
        *path = NULL;
        return -1;
    }

    (*path)->hop_count = 1;
    (*path)->total_hops = 0;
    (*path)->total_latency = 0;
    (*path)->vlan_hops = 0;

    return 0;
}

void memshadow_vlan_relay_path_cleanup(memshadow_vlan_relay_path_t *path) {
    if (path) {
        free(path->source);
        free(path->destination);

        if (path->hops) {
            for (size_t i = 0; i < path->hop_count; i++) {
                free(path->hops[i]);
            }
            free(path->hops);
        }

        memset(path, 0, sizeof(memshadow_vlan_relay_path_t));
    }
}

int memshadow_vlan_relay_path_add_hop(
    memshadow_vlan_relay_path_t *path,
    const char *node_id
) {
    if (!path || !node_id) {
        return -1;
    }

    // Reallocate hops array
    char **new_hops = realloc(path->hops, (path->hop_count + 1) * sizeof(char*));
    if (!new_hops) {
        return -1;
    }

    path->hops = new_hops;
    path->hops[path->hop_count] = strdup_safe(node_id);

    if (!path->hops[path->hop_count]) {
        return -1;
    }

    path->hop_count++;
    path->total_hops = path->hop_count - 1; // Don't count source

    // Check if hop is within same VLAN (simplified)
    if (path->hop_count >= 2) {
        // In real implementation, check actual VLAN topology
        path->vlan_hops++;
    }

    return 0;
}

/* VLAN Relay Manager Functions */
int memshadow_vlan_relay_manager_init(
    memshadow_vlan_relay_manager_t **manager,
    const char *node_id,
    int32_t vlan_id
) {
    if (!manager || !node_id) {
        return -1;
    }

    *manager = calloc(1, sizeof(memshadow_vlan_relay_manager_t));
    if (!*manager) {
        return -1;
    }

    (*manager)->node_id = strdup_safe(node_id);
    if (!(*manager)->node_id) {
        free(*manager);
        *manager = NULL;
        return -1;
    }

    (*manager)->vlan_id = vlan_id;
    (*manager)->node_capacity = 16; // Initial capacity
    (*manager)->nodes = calloc((*manager)->node_capacity, sizeof(memshadow_vlan_node_t));
    (*manager)->connections = calloc((*manager)->node_capacity, sizeof(char**));
    (*manager)->connection_counts = calloc((*manager)->node_capacity, sizeof(size_t));
    (*manager)->max_path_length = 10;

    if (!(*manager)->nodes || !(*manager)->connections || !(*manager)->connection_counts) {
        memshadow_vlan_relay_manager_cleanup(*manager);
        free(*manager);
        *manager = NULL;
        return -1;
    }

    return 0;
}

void memshadow_vlan_relay_manager_cleanup(memshadow_vlan_relay_manager_t *manager) {
    if (manager) {
        free(manager->node_id);

        if (manager->nodes) {
            for (size_t i = 0; i < manager->node_count; i++) {
                memshadow_vlan_node_cleanup(&manager->nodes[i]);
            }
            free(manager->nodes);
        }

        if (manager->connections) {
            for (size_t i = 0; i < manager->node_capacity; i++) {
                if (manager->connections[i]) {
                    for (size_t j = 0; j < manager->connection_counts[i]; j++) {
                        free(manager->connections[i][j]);
                    }
                    free(manager->connections[i]);
                }
            }
            free(manager->connections);
        }

        free(manager->connection_counts);
        memset(manager, 0, sizeof(memshadow_vlan_relay_manager_t));
    }
}

int memshadow_vlan_relay_manager_register_node(
    memshadow_vlan_relay_manager_t *manager,
    memshadow_vlan_node_t node
) {
    if (!manager) {
        return -1;
    }

    // Expand capacity if needed
    if (manager->node_count >= manager->node_capacity) {
        size_t new_capacity = manager->node_capacity * 2;
        memshadow_vlan_node_t *new_nodes = realloc(manager->nodes,
            new_capacity * sizeof(memshadow_vlan_node_t));
        char ***new_connections = realloc(manager->connections,
            new_capacity * sizeof(char**));
        size_t *new_counts = realloc(manager->connection_counts,
            new_capacity * sizeof(size_t));

        if (!new_nodes || !new_connections || !new_counts) {
            free(new_nodes);
            free(new_connections);
            free(new_counts);
            return -1;
        }

        // Initialize new connection arrays
        memset(new_connections + manager->node_capacity, 0,
            (new_capacity - manager->node_capacity) * sizeof(char**));
        memset(new_counts + manager->node_capacity, 0,
            (new_capacity - manager->node_capacity) * sizeof(size_t));

        manager->nodes = new_nodes;
        manager->connections = new_connections;
        manager->connection_counts = new_counts;
        manager->node_capacity = new_capacity;
    }

    manager->nodes[manager->node_count] = node;
    manager->node_count++;

    return 0;
}

int memshadow_vlan_relay_manager_register_connection(
    memshadow_vlan_relay_manager_t *manager,
    const char *from_node,
    const char *to_node
) {
    if (!manager || !from_node || !to_node) {
        return -1;
    }

    // Find from_node index
    size_t from_idx = SIZE_MAX;
    for (size_t i = 0; i < manager->node_count; i++) {
        if (strcmp(manager->nodes[i].node_id, from_node) == 0) {
            from_idx = i;
            break;
        }
    }

    if (from_idx == SIZE_MAX) {
        return -1; // from_node not found
    }

    // Find to_node index
    size_t to_idx = SIZE_MAX;
    for (size_t i = 0; i < manager->node_count; i++) {
        if (strcmp(manager->nodes[i].node_id, to_node) == 0) {
            to_idx = i;
            break;
        }
    }

    if (to_idx == SIZE_MAX) {
        return -1; // to_node not found
    }

    // Add bidirectional connection
    // Add to_node to from_node's connections
    char **new_from_connections = realloc(manager->connections[from_idx],
        (manager->connection_counts[from_idx] + 1) * sizeof(char*));
    if (!new_from_connections) {
        return -1;
    }

    manager->connections[from_idx] = new_from_connections;
    manager->connections[from_idx][manager->connection_counts[from_idx]] =
        strdup_safe(to_node);
    if (!manager->connections[from_idx][manager->connection_counts[from_idx]]) {
        return -1;
    }
    manager->connection_counts[from_idx]++;

    // Add from_node to to_node's connections
    char **new_to_connections = realloc(manager->connections[to_idx],
        (manager->connection_counts[to_idx] + 1) * sizeof(char*));
    if (!new_to_connections) {
        return -1;
    }

    manager->connections[to_idx] = new_to_connections;
    manager->connections[to_idx][manager->connection_counts[to_idx]] =
        strdup_safe(from_node);
    if (!manager->connections[to_idx][manager->connection_counts[to_idx]]) {
        return -1;
    }
    manager->connection_counts[to_idx]++;

    return 0;
}

void memshadow_vlan_relay_manager_get_topology_summary(
    const memshadow_vlan_relay_manager_t *manager,
    memshadow_vlan_topology_summary_t *summary
) {
    if (!manager || !summary) {
        return;
    }

    memset(summary, 0, sizeof(memshadow_vlan_topology_summary_t));

    summary->total_nodes = manager->node_count;

    // Count connectivity types and relay status
    for (size_t i = 0; i < manager->node_count; i++) {
        const memshadow_vlan_node_t *node = &manager->nodes[i];

        if (memshadow_vlan_connectivity_has_internet_access(node->connectivity)) {
            summary->internet_nodes++;
        }

        if (memshadow_vlan_connectivity_can_relay(node->connectivity)) {
            summary->relay_nodes++;
            if (node->relay_load > 0) {
                summary->active_relays++;
            }
        }
    }

    // Count unique VLANs (simplified - just use the manager's VLAN)
    summary->vlan_count = 1;
}

int memshadow_vlan_relay_manager_find_relay_path(
    memshadow_vlan_relay_manager_t *manager,
    const char *destination,
    bool prefer_vlan,
    memshadow_vlan_relay_path_t **path
) {
    if (!manager || !destination || !path) {
        return -1;
    }

    // BFS implementation
    char **visited = calloc(manager->node_count, sizeof(char*));
    char **parent_map = calloc(manager->node_count, sizeof(char*));
    char **queue = calloc(manager->node_count, sizeof(char*));
    size_t queue_start = 0;
    size_t queue_end = 0;

    if (!visited || !parent_map || !queue) {
        free(visited);
        free(parent_map);
        free(queue);
        return -1;
    }

    // Find starting node (our node)
    size_t start_idx = SIZE_MAX;
    for (size_t i = 0; i < manager->node_count; i++) {
        if (strcmp(manager->nodes[i].node_id, manager->node_id) == 0) {
            start_idx = i;
            break;
        }
    }

    if (start_idx == SIZE_MAX) {
        free(visited);
        free(parent_map);
        free(queue);
        return -1;
    }

    // Start BFS
    queue[queue_end++] = manager->nodes[start_idx].node_id;
    visited[start_idx] = (char*)1; // Mark as visited

    bool found = false;
    while (queue_start < queue_end && !found) {
        const char *current = queue[queue_start++];
        size_t current_idx = SIZE_MAX;

        // Find current node index
        for (size_t i = 0; i < manager->node_count; i++) {
            if (strcmp(manager->nodes[i].node_id, current) == 0) {
                current_idx = i;
                break;
            }
        }

        if (current_idx == SIZE_MAX) continue;

        // Check if we reached destination
        if (strcmp(current, destination) == 0) {
            found = true;
            break;
        }

        // Explore neighbors
        for (size_t i = 0; i < manager->connection_counts[current_idx]; i++) {
            const char *neighbor = manager->connections[current_idx][i];
            size_t neighbor_idx = SIZE_MAX;

            // Find neighbor index
            for (size_t j = 0; j < manager->node_count; j++) {
                if (strcmp(manager->nodes[j].node_id, neighbor) == 0) {
                    neighbor_idx = j;
                    break;
                }
            }

            if (neighbor_idx != SIZE_MAX && !visited[neighbor_idx]) {
                visited[neighbor_idx] = (char*)1;
                parent_map[neighbor_idx] = (char*)current; // Store parent
                queue[queue_end++] = (char*)neighbor;

                // Limit path length
                if (queue_end - queue_start > manager->max_path_length) {
                    break;
                }
            }
        }
    }

    int result = -1;

    if (found) {
        // Reconstruct path
        if (memshadow_vlan_relay_path_init(path, manager->node_id, destination) == 0) {
            // Find destination index
            size_t dest_idx = SIZE_MAX;
            for (size_t i = 0; i < manager->node_count; i++) {
                if (strcmp(manager->nodes[i].node_id, destination) == 0) {
                    dest_idx = i;
                    break;
                }
            }

            if (dest_idx != SIZE_MAX) {
                // Reconstruct path by following parent pointers
                size_t current_idx = dest_idx;
                while (current_idx != start_idx) {
                    const char *parent = parent_map[current_idx];
                    if (!parent) break;

                    // Add to path (in reverse, will be reversed later)
                    if (memshadow_vlan_relay_path_add_hop(*path, manager->nodes[current_idx].node_id) != 0) {
                        break;
                    }

                    // Find parent index
                    for (size_t i = 0; i < manager->node_count; i++) {
                        if (strcmp(manager->nodes[i].node_id, parent) == 0) {
                            current_idx = i;
                            break;
                        }
                    }
                }

                result = 0;
            }
        }
    }

    // Cleanup
    free(visited);
    free(parent_map);
    free(queue);

    return result;
}

int32_t memshadow_vlan_relay_manager_get_relay_load(
    const memshadow_vlan_relay_manager_t *manager,
    const char *node_id
) {
    if (!manager || !node_id) {
        return -1;
    }

    for (size_t i = 0; i < manager->node_count; i++) {
        if (strcmp(manager->nodes[i].node_id, node_id) == 0) {
            return manager->nodes[i].relay_load;
        }
    }

    return -1; // Node not found
}

int memshadow_vlan_relay_manager_update_relay_load(
    memshadow_vlan_relay_manager_t *manager,
    const char *node_id,
    int32_t new_load
) {
    if (!manager || !node_id) {
        return -1;
    }

    for (size_t i = 0; i < manager->node_count; i++) {
        if (strcmp(manager->nodes[i].node_id, node_id) == 0) {
            manager->nodes[i].relay_load = new_load;
            return 0;
        }
    }

    return -1; // Node not found
}