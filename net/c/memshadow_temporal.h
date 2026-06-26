/**
 * MEMSHADOW Protocol v3.0 - Temporal Queue (C)
 *
 * Time-delayed message delivery with TTL, priority, and cancellation.
 */

#ifndef MEMSHADOW_TEMPORAL_H
#define MEMSHADOW_TEMPORAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMSHADOW_TEMPORAL_MAX  4096
#define MEMSHADOW_TEMPORAL_ID   32

typedef enum {
    TEMPORAL_SCHEDULED = 0,
    TEMPORAL_DELAYED = 1,
    TEMPORAL_CONDITIONAL = 2,
    TEMPORAL_PRIORITY_QUEUE = 3,
} memshadow_temporal_mode_t;

typedef struct {
    char     id[MEMSHADOW_TEMPORAL_ID + 1];
    uint8_t *payload;
    size_t   payload_size;
    char     destination[64];
    memshadow_temporal_mode_t mode;
    uint64_t deliver_at_ns;
    uint64_t created_at_ns;
    uint64_t ttl_ms;
    uint8_t  priority;
    bool     cancelled;
    bool     delivered;
    uint32_t retry_count;
    uint32_t max_retries;
} memshadow_temporal_msg_t;

typedef struct {
    uint64_t enqueued;
    uint64_t delivered;
    uint64_t cancelled;
    uint64_t expired;
    uint64_t retried;
} memshadow_temporal_stats_t;

typedef struct {
    memshadow_temporal_msg_t *messages;
    uint32_t count;
    uint32_t max_size;
    memshadow_temporal_stats_t stats;
} memshadow_temporal_queue_t;

int memshadow_temporal_init(memshadow_temporal_queue_t *queue, uint32_t max_size);
void memshadow_temporal_destroy(memshadow_temporal_queue_t *queue);

int memshadow_temporal_enqueue(memshadow_temporal_queue_t *queue,
                                const char *id,
                                const uint8_t *payload, size_t payload_size,
                                const char *destination,
                                memshadow_temporal_mode_t mode,
                                uint64_t delay_ms,
                                uint8_t priority);

int memshadow_temporal_cancel(memshadow_temporal_queue_t *queue, const char *id);

/* Get ready messages. Returns count. Caller must process and free payloads. */
uint32_t memshadow_temporal_get_ready(memshadow_temporal_queue_t *queue,
                                       memshadow_temporal_msg_t *out_msgs,
                                       uint32_t max_msgs);

int memshadow_temporal_retry(memshadow_temporal_queue_t *queue,
                              const char *id, uint64_t delay_ms);

void memshadow_temporal_cleanup(memshadow_temporal_queue_t *queue);

uint32_t memshadow_temporal_ready_count(const memshadow_temporal_queue_t *queue);
uint32_t memshadow_temporal_pending_count(const memshadow_temporal_queue_t *queue);
void memshadow_temporal_get_stats(const memshadow_temporal_queue_t *queue,
                                   memshadow_temporal_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_TEMPORAL_H */
