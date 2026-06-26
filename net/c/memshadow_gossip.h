/**
 * MEMSHADOW Protocol - Gossip Protocol Header
 *
 * Implements epidemic broadcast and anti-entropy mechanisms for decentralized
 * peer-to-peer communication. Supports probabilistic gossip, fan-out control,
 * and message deduplication.
 */

#ifndef MEMSHADOW_GOSSIP_H
#define MEMSHADOW_GOSSIP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Gossip Message Types */
typedef enum {
    GOSSIP_MSG_HEARTBEAT = 0,
    GOSSIP_MSG_PEER_DISCOVERY = 1,
    GOSSIP_MSG_ROUTE_UPDATE = 2,
    GOSSIP_MSG_DATA_BROADCAST = 3,
    GOSSIP_MSG_FAILURE_DETECTION = 4,
    GOSSIP_MSG_METADATA_SYNC = 5,
} memshadow_gossip_message_type_t;

/* Gossip Peer Information */
typedef struct {
    char *peer_id;
    char *address;
    uint16_t port;
    uint64_t last_seen;
    uint32_t heartbeat_count;
    float suspicion_level;
} memshadow_gossip_peer_t;

/* Gossip Message */
typedef struct {
    uint8_t id[16];  // UUID for deduplication
    memshadow_gossip_message_type_t msg_type;
    char *sender_id;
    uint8_t *payload;
    size_t payload_size;
    uint64_t timestamp;
    uint32_t ttl;
    uint32_t hop_count;
} memshadow_gossip_message_t;

/* Gossip Manager */
typedef struct {
    char *node_id;
    memshadow_gossip_peer_t *peers;
    size_t peer_count;
    size_t peer_capacity;
    uint8_t (*seen_messages)[16];  // Array of message IDs
    uint64_t *seen_timestamps;
    size_t seen_count;
    size_t seen_capacity;
    memshadow_gossip_message_t *outgoing_queue;
    size_t outgoing_count;
    size_t outgoing_capacity;
    uint32_t fan_out;
    uint64_t gossip_interval_ms;
    uint64_t last_gossip_time;
    uint32_t random_seed;
} memshadow_gossip_manager_t;

/* Gossip Statistics */
typedef struct {
    size_t total_peers;
    size_t seen_messages;
    size_t queued_messages;
    size_t suspected_failures;
} memshadow_gossip_statistics_t;

/* Outgoing Message */
typedef struct {
    char *peer_id;
    memshadow_gossip_message_t message;
} memshadow_gossip_outgoing_message_t;

/* Function Declarations */

/* Initialize gossip peer */
int memshadow_gossip_peer_init(
    memshadow_gossip_peer_t *peer,
    const char *peer_id,
    const char *address,
    uint16_t port
);

/* Cleanup gossip peer */
void memshadow_gossip_peer_cleanup(memshadow_gossip_peer_t *peer);

/* Update peer last seen */
void memshadow_gossip_peer_update_last_seen(memshadow_gossip_peer_t *peer);

/* Increase peer suspicion */
void memshadow_gossip_peer_increase_suspicion(memshadow_gossip_peer_t *peer);

/* Check if peer is suspected failed */
bool memshadow_gossip_peer_is_suspected_failed(
    const memshadow_gossip_peer_t *peer,
    float threshold
);

/* Initialize gossip message */
int memshadow_gossip_message_init(
    memshadow_gossip_message_t *message,
    memshadow_gossip_message_type_t msg_type,
    const char *sender_id,
    const uint8_t *payload,
    size_t payload_size,
    uint32_t ttl
);

/* Cleanup gossip message */
void memshadow_gossip_message_cleanup(memshadow_gossip_message_t *message);

/* Check if message is expired */
bool memshadow_gossip_message_is_expired(const memshadow_gossip_message_t *message);

/* Decrement message TTL */
void memshadow_gossip_message_decrement_ttl(memshadow_gossip_message_t *message);

/* Check if message IDs are equal */
bool memshadow_gossip_message_id_equals(
    const uint8_t id1[16],
    const uint8_t id2[16]
);

/* Initialize gossip manager */
int memshadow_gossip_manager_init(
    memshadow_gossip_manager_t **manager,
    const char *node_id,
    uint32_t fan_out,
    uint64_t gossip_interval_ms
);

/* Cleanup gossip manager */
void memshadow_gossip_manager_cleanup(memshadow_gossip_manager_t *manager);

/* Add peer to gossip network */
int memshadow_gossip_manager_add_peer(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_peer_t peer
);

/* Remove peer from gossip network */
bool memshadow_gossip_manager_remove_peer(
    memshadow_gossip_manager_t *manager,
    const char *peer_id
);

/* Broadcast message using gossip */
int memshadow_gossip_manager_broadcast_message(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_message_type_t msg_type,
    const uint8_t *payload,
    size_t payload_size,
    uint32_t ttl
);

/* Receive gossip message */
int memshadow_gossip_manager_receive_message(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_message_t message,
    const char *from_peer
);

/* Perform gossip round */
int memshadow_gossip_manager_perform_gossip_round(
    memshadow_gossip_manager_t *manager,
    memshadow_gossip_outgoing_message_t **outgoing_messages,
    size_t *message_count
);

/* Cleanup outgoing messages array */
void memshadow_gossip_outgoing_messages_cleanup(
    memshadow_gossip_outgoing_message_t *messages,
    size_t count
);

/* Run failure detection */
int memshadow_gossip_manager_run_failure_detection(
    memshadow_gossip_manager_t *manager
);

/* Get gossip statistics */
void memshadow_gossip_manager_get_statistics(
    const memshadow_gossip_manager_t *manager,
    memshadow_gossip_statistics_t *statistics
);

/* Clean up old seen messages */
void memshadow_gossip_manager_cleanup_old_messages(
    memshadow_gossip_manager_t *manager,
    uint64_t max_age_ns
);

/* Perform anti-entropy synchronization */
int memshadow_gossip_manager_perform_anti_entropy(
    memshadow_gossip_manager_t *manager,
    const uint8_t (*remote_message_ids)[16],
    size_t remote_count,
    memshadow_gossip_message_t **missing_messages,
    size_t *missing_count
);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_GOSSIP_H */
