/**
 * MEMSHADOW Protocol - Gossip Protocol Implementation
 *
 * Implements epidemic broadcast and anti-entropy mechanisms for decentralized
 * peer-to-peer communication. Supports probabilistic gossip, fan-out control,
 * and message deduplication.
 */

#include "memshadow_gossip.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Helper functions */
static char *strdup_safe(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

static uint8_t *memdup(const uint8_t *data, size_t size) {
    if (!data || size == 0) return NULL;
    uint8_t *dup = malloc(size);
    if (dup) {
        memcpy(dup, data, size);
    }
    return dup;
}

static uint32_t simple_rand(uint32_t *seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed & 0x7fffffff;
}

static void generate_uuid(uint8_t uuid[16]) {
    if (!uuid) return;

    uint32_t seed = (uint32_t)time(NULL);
    for (int i = 0; i < 16; i++) {
        uuid[i] = (uint8_t)(simple_rand(&seed) % 256);
    }
}

/* Gossip Peer Functions */
int memshadow_gossip_peer_init(
    memshadow_gossip_peer_t *peer,
    const char *peer_id,
    const char *address,
    uint16_t port
) {
    if (!peer || !peer_id || !address) return -1;

    memset(peer, 0, sizeof(memshadow_gossip_peer_t));

    peer->peer_id = strdup_safe(peer_id);
    peer->address = strdup_safe(address);
    peer->port = port;
    peer->last_seen = (uint64_t)time(NULL) * 1000000000ULL;
    peer->heartbeat_count = 0;
    peer->suspicion_level = 0.0f;

    if (!peer->peer_id || !peer->address) {
        memshadow_gossip_peer_cleanup(peer);
        return -1;
    }

    return 0;
}

void memshadow_gossip_peer_cleanup(memshadow_gossip_peer_t *peer) {
    if (peer) {
        free(peer->peer_id);
        free(peer->address);
        memset(peer, 0, sizeof(memshadow_gossip_peer_t));
    }
}

void memshadow_gossip_peer_update_last_seen(memshadow_gossip_peer_t *peer) {
    if (peer) {
        peer->last_seen = (uint64_t)time(NULL) * 1000000000ULL;
        peer->heartbeat_count++;
        peer->suspicion_level = peer->suspicion_level > 0.1f ?
            peer->suspicion_level - 0.1f : 0.0f;
    }
}

void memshadow_gossip_peer_increase_suspicion(memshadow_gossip_peer_t *peer) {
    if (peer) {
        peer->suspicion_level = peer->suspicion_level < 1.0f ?
            peer->suspicion_level + 0.2f : 1.0f;
    }
}

bool memshadow_gossip_peer_is_suspected_failed(
    const memshadow_gossip_peer_t *peer,
    float threshold
) {
    return peer && peer->suspicion_level >= threshold;
}

/* Gossip Message Functions */
int memshadow_gossip_message_init(
    memshadow_gossip_message_t *message,
    memshadow_gossip_message_type_t msg_type,
    const char *sender_id,
    const uint8_t *payload,
    size_t payload_size,
    uint32_t ttl
) {
    if (!message || !sender_id) return -1;

    memset(message, 0, sizeof(memshadow_gossip_message_t));

    generate_uuid(message->id);
    message->msg_type = msg_type;
    message->sender_id = strdup_safe(sender_id);
    message->payload = payload_size > 0 ? memdup(payload, payload_size) : NULL;
    message->payload_size = payload_size;
    message->timestamp = (uint64_t)time(NULL) * 1000000000ULL;
    message->ttl = ttl;
    message->hop_count = 0;

    if (!message->sender_id || (payload_size > 0 && !message->payload)) {
        memshadow_gossip_message_cleanup(message);
        return -1;
    }

    return 0;
}

void memshadow_gossip_message_cleanup(memshadow_gossip_message_t *message) {
    if (message) {
        free(message->sender_id);
        free(message->payload);
        memset(message, 0, sizeof(memshadow_gossip_message_t));
    }
}

bool memshadow_gossip_message_is_expired(const memshadow_gossip_message_t *message) {
    return message && message->ttl == 0;
}

void memshadow_gossip_message_decrement_ttl(memshadow_gossip_message_t *message) {
    if (message && message->ttl > 0) {
        message->ttl--;
        message->hop_count++;
    }
}

bool memshadow_gossip_message_id_equals(const uint8_t id1[16], const uint8_t id2[16]) {
    return id1 && id2 && memcmp(id1, id2, 16) == 0;
}

/* Gossip Manager Functions */
int memshadow_gossip_manager_init(
    memshadow_gossip_manager_t **manager,
    const char *node_id,
    uint32_t fan_out,
    uint64_t gossip_interval_ms
) {
    if (!manager || !node_id) return -1;

    *manager = calloc(1, sizeof(memshadow_gossip_manager_t));
    if (!*manager) return -1;

    (*manager)->node_id = strdup_safe(node_id);
    if (!(*manager)->node_id) {
        free(*manager);
        return -1;
    }

    (*manager)->peer_capacity = 32;
    (*manager)->peers = calloc((*manager)->peer_capacity, sizeof(memshadow_gossip_peer_t));

    (*manager)->seen_capacity = 1024;
    (*manager)->seen_messages = calloc((*manager)->seen_capacity, sizeof(uint8_t[16]));
    (*manager)->seen_timestamps = calloc((*manager)->seen_capacity, sizeof(uint64_t));

    (*manager)->outgoing_capacity = 256;
    (*manager)->outgoing_queue = calloc((*manager)->outgoing_capacity, sizeof(memshadow_gossip_message_t));

    (*manager)->fan_out = fan_out > 0 ? fan_out : 3;
    (*manager)->gossip_interval_ms = gossip_interval_ms > 0 ? gossip_interval_ms : 1000;
    (*manager)->last_gossip_time = 0;
    (*manager)->random_seed = (uint32_t)time(NULL);

    if (!(*manager)->peers || !(*manager)->seen_messages || !(*manager)->seen_timestamps ||
        !(*manager)->outgoing_queue) {
        memshadow_gossip_manager_cleanup(*manager);
        return -1;
    }

    return 0;
}

void memshadow_gossip_manager_cleanup(memshadow_gossip_manager_t *manager) {
    if (manager) {
        free(manager->node_id);

        if (manager->peers) {
            for (size_t i = 0; i < manager->peer_count; i++) {
                memshadow_gossip_peer_cleanup(&manager->peers[i]);
            }
            free(manager->peers);
        }

        free(manager->seen_messages);
        free(manager->seen_timestamps);

        if (manager->outgoing_queue) {
            for (size_t i = 0; i < manager->outgoing_count; i++) {
                memshadow_gossip_message_cleanup(&manager->outgoing_queue[i]);
            }
            free(manager->outgoing_queue);
        }

        memset(manager, 0, sizeof(memshadow_gossip_manager_t));
        free(manager);
    }
}

int memshadow_gossip_manager_add_peer(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_peer_t peer
) {
    if (!manager) return -1;

    // Check if peer already exists
    for (size_t i = 0; i < manager->peer_count; i++) {
        if (strcmp(manager->peers[i].peer_id, peer.peer_id) == 0) {
            memshadow_gossip_peer_cleanup(&peer);
            return 0; // Already exists
        }
    }

    // Expand capacity if needed
    if (manager->peer_count >= manager->peer_capacity) {
        size_t new_capacity = manager->peer_capacity * 2;
        memshadow_gossip_peer_t *new_peers = realloc(manager->peers,
            new_capacity * sizeof(memshadow_gossip_peer_t));

        if (!new_peers) {
            memshadow_gossip_peer_cleanup(&peer);
            return -1;
        }

        memset(new_peers + manager->peer_capacity, 0,
            (new_capacity - manager->peer_capacity) * sizeof(memshadow_gossip_peer_t));

        manager->peers = new_peers;
        manager->peer_capacity = new_capacity;
    }

    manager->peers[manager->peer_count++] = peer;
    return 0;
}

bool memshadow_gossip_manager_remove_peer(
    memshadow_gossip_manager_t *manager,
    const char *peer_id
) {
    if (!manager || !peer_id) return false;

    for (size_t i = 0; i < manager->peer_count; i++) {
        if (strcmp(manager->peers[i].peer_id, peer_id) == 0) {
            memshadow_gossip_peer_cleanup(&manager->peers[i]);

            // Shift remaining peers
            for (size_t j = i; j < manager->peer_count - 1; j++) {
                manager->peers[j] = manager->peers[j + 1];
            }

            manager->peer_count--;
            memset(&manager->peers[manager->peer_count], 0, sizeof(memshadow_gossip_peer_t));
            return true;
        }
    }

    return false;
}

int memshadow_gossip_manager_broadcast_message(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_message_type_t msg_type,
    const uint8_t *payload,
    size_t payload_size,
    uint32_t ttl
) {
    if (!manager) return -1;

    // Check if we have space in outgoing queue
    if (manager->outgoing_count >= manager->outgoing_capacity) {
        return -1; // Queue full
    }

    // Create message
    memshadow_gossip_message_t message;
    if (memshadow_gossip_message_init(&message, msg_type, manager->node_id,
        payload, payload_size, ttl) != 0) {
        return -1;
    }

    // Mark as seen by us using ring buffer
    size_t index = manager->seen_count % manager->seen_capacity;
    memcpy(manager->seen_messages[index], message.id, 16);
    manager->seen_timestamps[index] = message.timestamp;
    manager->seen_count++;

    // Add to outgoing queue
    manager->outgoing_queue[manager->outgoing_count++] = message;

    return 0;
}

int memshadow_gossip_manager_receive_message(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_message_t message,
    const char *from_peer
) {
    if (!manager || !from_peer) {
        memshadow_gossip_message_cleanup(&message);
        return -1;
    }

    // Check if we've already seen this message
    size_t check_count = (manager->seen_count < manager->seen_capacity) ? manager->seen_count : manager->seen_capacity;
    for (size_t i = 0; i < check_count; i++) {
        if (memshadow_gossip_message_id_equals(message.id, manager->seen_messages[i])) {
            memshadow_gossip_message_cleanup(&message);
            return 0; // Duplicate, ignore
        }
    }

    // Check if message is expired
    if (memshadow_gossip_message_is_expired(&message)) {
        memshadow_gossip_message_cleanup(&message);
        return 0;
    }

    // Mark as seen using a ring buffer to prevent cache exhaustion DoS
    size_t index = manager->seen_count % manager->seen_capacity;
    memcpy(manager->seen_messages[index], message.id, 16);
    manager->seen_timestamps[index] = message.timestamp;
    manager->seen_count++;

    // Update sender peer information
    for (size_t i = 0; i < manager->peer_count; i++) {
        if (strcmp(manager->peers[i].peer_id, from_peer) == 0) {
            memshadow_gossip_peer_update_last_seen(&manager->peers[i]);
            break;
        }
    }

    // Process message based on type
    switch (message.msg_type) {
        case GOSSIP_MSG_HEARTBEAT:
            // Message will be naturally rebroadcasted via TTL logic below
            break;

        case GOSSIP_MSG_PEER_DISCOVERY:
            // Process peer discovery
            if (message.payload && message.payload_size > 0) {
                // Payload format: "peer_id:address:port"
                char *payload_str = malloc(message.payload_size + 1);
                if (payload_str) {
                    memcpy(payload_str, message.payload, message.payload_size);
                    payload_str[message.payload_size] = '\0';

                    char *peer_id = strtok(payload_str, ":");
                    char *address = strtok(NULL, ":");
                    char *port_str = strtok(NULL, ":");

                    if (peer_id && address && port_str) {
                        uint16_t port = (uint16_t)atoi(port_str);

                        // Check if we already know this peer
                        bool known = false;
                        for (size_t i = 0; i < manager->peer_count; i++) {
                            if (strcmp(manager->peers[i].peer_id, peer_id) == 0) {
                                known = true;
                                break;
                            }
                        }

                        if (!known) {
                            memshadow_gossip_peer_t new_peer;
                            if (memshadow_gossip_peer_init(&new_peer, peer_id, address, port) == 0) {
                                memshadow_gossip_manager_add_peer(manager, new_peer);
                            }
                        }
                    }

                    free(payload_str);
                }
            }
            break;

        case GOSSIP_MSG_DATA_BROADCAST:
            // Handle data broadcast - in real implementation, pass to application
            break;

        case GOSSIP_MSG_FAILURE_DETECTION:
            // Process failure detection
            if (message.payload && message.payload_size > 0) {
                char *failed_peer_id = malloc(message.payload_size + 1);
                if (failed_peer_id) {
                    memcpy(failed_peer_id, message.payload, message.payload_size);
                    failed_peer_id[message.payload_size] = '\0';

                    // Increase suspicion for this peer
                    for (size_t i = 0; i < manager->peer_count; i++) {
                        if (strcmp(manager->peers[i].peer_id, failed_peer_id) == 0) {
                            memshadow_gossip_peer_increase_suspicion(&manager->peers[i]);
                            break;
                        }
                    }

                    free(failed_peer_id);
                }
            }
            break;

        default:
            // Handle other message types
            break;
    }

    // Decrement TTL and rebroadcast if still valid
    memshadow_gossip_message_decrement_ttl(&message);

    if (!memshadow_gossip_message_is_expired(&message)) {
        // Add to outgoing queue for rebroadcast
        if (manager->outgoing_count < manager->outgoing_capacity) {
            manager->outgoing_queue[manager->outgoing_count++] = message;
            return 0; // Don't cleanup, message is now in queue
        }
    }

    // Cleanup message
    memshadow_gossip_message_cleanup(&message);
    return 0;
}

int memshadow_gossip_manager_perform_gossip_round(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_outgoing_message_t **outgoing_messages,
    size_t *message_count
) {
    if (!manager || !outgoing_messages || !message_count) return -1;

    if (manager->peer_count == 0 || manager->outgoing_count == 0) {
        *outgoing_messages = NULL;
        *message_count = 0;
        return 0;
    }

    // Select random peers to gossip to
    size_t gossip_targets = manager->peer_count < manager->fan_out ?
        manager->peer_count : manager->fan_out;

    size_t *selected_indices = malloc(gossip_targets * sizeof(size_t));
    if (!selected_indices) return -1;

    // Fisher-Yates shuffle to select random peers
    for (size_t i = 0; i < manager->peer_count; i++) {
        selected_indices[i % gossip_targets] = i;
    }

    for (size_t i = gossip_targets; i < manager->peer_count; i++) {
        size_t j = simple_rand(&manager->random_seed) % (i + 1);
        if (j < gossip_targets) {
            selected_indices[j] = i;
        }
    }

    // Create outgoing messages
    *message_count = gossip_targets * manager->outgoing_count;
    *outgoing_messages = calloc(*message_count, sizeof(memshadow_gossip_outgoing_message_t));

    if (!*outgoing_messages) {
        free(selected_indices);
        return -1;
    }

    size_t msg_idx = 0;
    for (size_t i = 0; i < gossip_targets; i++) {
        size_t peer_idx = selected_indices[i];

        for (size_t j = 0; j < manager->outgoing_count; j++) {
            (*outgoing_messages)[msg_idx].peer_id = strdup_safe(manager->peers[peer_idx].peer_id);
            if (!(*outgoing_messages)[msg_idx].peer_id) {
                memshadow_gossip_outgoing_messages_cleanup(*outgoing_messages, msg_idx);
                free(selected_indices);
                return -1;
            }

            // Copy message
            memshadow_gossip_message_t *src = &manager->outgoing_queue[j];
            memshadow_gossip_message_t *dst = &(*outgoing_messages)[msg_idx].message;

            memcpy(dst->id, src->id, 16);
            dst->msg_type = src->msg_type;
            dst->sender_id = strdup_safe(src->sender_id);
            dst->payload = src->payload_size > 0 ? memdup(src->payload, src->payload_size) : NULL;
            dst->payload_size = src->payload_size;
            dst->timestamp = src->timestamp;
            dst->ttl = src->ttl;
            dst->hop_count = src->hop_count;

            if ((dst->payload_size > 0 && !dst->payload) || !dst->sender_id) {
                memshadow_gossip_outgoing_messages_cleanup(*outgoing_messages, msg_idx + 1);
                free(selected_indices);
                return -1;
            }

            msg_idx++;
        }
    }

    // Clear outgoing queue after sending
    for (size_t i = 0; i < manager->outgoing_count; i++) {
        memshadow_gossip_message_cleanup(&manager->outgoing_queue[i]);
    }
    manager->outgoing_count = 0;

    free(selected_indices);
    return 0;
}

void memshadow_gossip_outgoing_messages_cleanup(
    memshadow_gossip_outgoing_message_t *messages,
    size_t count
) {
    if (messages) {
        for (size_t i = 0; i < count; i++) {
            free(messages[i].peer_id);
            memshadow_gossip_message_cleanup(&messages[i].message);
        }
        free(messages);
    }
}

int memshadow_gossip_manager_run_failure_detection(
    memshadow_gossip_manager_t *manager
) {
    if (!manager) return -1;

    uint64_t current_time = (uint64_t)time(NULL) * 1000000000ULL;
    uint64_t timeout_ns = 30ULL * 1000000000ULL; // 30 seconds

    for (size_t i = 0; i < manager->peer_count; i++) {
        if (current_time - manager->peers[i].last_seen > timeout_ns) {
            memshadow_gossip_peer_increase_suspicion(&manager->peers[i]);

            // If suspicion is high, broadcast failure detection
            if (memshadow_gossip_peer_is_suspected_failed(&manager->peers[i], 0.8f)) {
                memshadow_gossip_manager_broadcast_message(manager, GOSSIP_MSG_FAILURE_DETECTION,
                    (const uint8_t*)manager->peers[i].peer_id,
                    strlen(manager->peers[i].peer_id), 5);
            }
        }
    }

    return 0;
}

void memshadow_gossip_manager_get_statistics(
    const memshadow_gossip_manager_t *manager,
    memshadow_gossip_statistics_t *statistics
) {
    if (!manager || !statistics) return;

    memset(statistics, 0, sizeof(memshadow_gossip_statistics_t));

    statistics->total_peers = manager->peer_count;
    statistics->seen_messages = manager->seen_count;
    statistics->queued_messages = manager->outgoing_count;

    // Count suspected failures
    for (size_t i = 0; i < manager->peer_count; i++) {
        if (memshadow_gossip_peer_is_suspected_failed(&manager->peers[i], 0.5f)) {
            statistics->suspected_failures++;
        }
    }
}

void memshadow_gossip_manager_cleanup_old_messages(
    memshadow_gossip_manager_t *manager,
    uint64_t max_age_ns
) {
    if (!manager) return;

    uint64_t current_time = (uint64_t)time(NULL) * 1000000000ULL;

    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < manager->seen_count; read_idx++) {
        if (current_time - manager->seen_timestamps[read_idx] <= max_age_ns) {
            if (write_idx != read_idx) {
                memcpy(manager->seen_messages[write_idx], manager->seen_messages[read_idx], 16);
                manager->seen_timestamps[write_idx] = manager->seen_timestamps[read_idx];
            }
            write_idx++;
        }
    }

    manager->seen_count = write_idx;
}

int memshadow_gossip_manager_perform_anti_entropy(
    memshadow_gossip_manager_t *manager,
    const uint8_t (*remote_message_ids)[16],
    size_t remote_count,
    memshadow_gossip_message_t **missing_messages,
    size_t *missing_count
) {
    if (!manager || !missing_messages || !missing_count) return -1;

    // For simplicity, this is a placeholder implementation
    // In a real implementation, this would compare message sets and return missing messages
    *missing_messages = NULL;
    *missing_count = 0;

    return 0;
}
