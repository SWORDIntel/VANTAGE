/**
 * MEMSHADOW Protocol v3.0 - Temporal Queue Implementation (C)
 */

#include "memshadow_temporal.h"
#include "memshadow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/random.h>
#endif

int memshadow_temporal_init(memshadow_temporal_queue_t *queue, uint32_t max_size) {
    if (!queue) return -1;
    memset(queue, 0, sizeof(*queue));
    queue->max_size = (max_size > 0) ? max_size : MEMSHADOW_TEMPORAL_MAX;
    queue->messages = calloc(queue->max_size, sizeof(memshadow_temporal_msg_t));
    if (!queue->messages) return -2;
    return 0;
}

void memshadow_temporal_destroy(memshadow_temporal_queue_t *queue) {
    if (!queue) return;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (queue->messages[i].payload) {
            free(queue->messages[i].payload);
        }
    }
    free(queue->messages);
    queue->messages = NULL;
    queue->count = 0;
}

int memshadow_temporal_enqueue(memshadow_temporal_queue_t *queue,
                                const char *id,
                                const uint8_t *payload, size_t payload_size,
                                const char *destination,
                                memshadow_temporal_mode_t mode,
                                uint64_t delay_ms,
                                uint8_t priority) {
    if (!queue || !id || !payload || !destination) return -1;

    if (queue->count >= queue->max_size) {
        memshadow_temporal_cleanup(queue);
        if (queue->count >= queue->max_size) return -2;
    }

    memshadow_temporal_msg_t *msg = &queue->messages[queue->count];
    memset(msg, 0, sizeof(*msg));
    strncpy(msg->id, id, MEMSHADOW_TEMPORAL_ID);
    msg->payload = malloc(payload_size);
    if (!msg->payload) return -3;
    memcpy(msg->payload, payload, payload_size);
    msg->payload_size = payload_size;
    strncpy(msg->destination, destination, sizeof(msg->destination) - 1);
    msg->mode = mode;
    msg->created_at_ns = memshadow_get_timestamp_ns();
    msg->deliver_at_ns = msg->created_at_ns + delay_ms * 1000000ULL;
    msg->ttl_ms = delay_ms + 3600000; /* delay + 1 hour */
    msg->priority = priority;
    msg->max_retries = 3;

    queue->count++;
    queue->stats.enqueued++;

    /* Sort by delivery time (insertion sort — small arrays) */
    for (uint32_t i = queue->count - 1; i > 0; i--) {
        if (queue->messages[i].deliver_at_ns < queue->messages[i - 1].deliver_at_ns) {
            memshadow_temporal_msg_t tmp = queue->messages[i];
            queue->messages[i] = queue->messages[i - 1];
            queue->messages[i - 1] = tmp;
        } else {
            break;
        }
    }

    return 0;
}

int memshadow_temporal_cancel(memshadow_temporal_queue_t *queue, const char *id) {
    if (!queue || !id) return -1;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (strcmp(queue->messages[i].id, id) == 0) {
            queue->messages[i].cancelled = true;
            queue->stats.cancelled++;
            return 0;
        }
    }
    return -2;
}

static bool msg_is_ready(const memshadow_temporal_msg_t *msg) {
    if (msg->cancelled || msg->delivered) return false;
    return memshadow_get_timestamp_ns() >= msg->deliver_at_ns;
}

static bool msg_is_expired(const memshadow_temporal_msg_t *msg) {
    uint64_t age_ms = (memshadow_get_timestamp_ns() - msg->created_at_ns) / 1000000ULL;
    return age_ms > msg->ttl_ms;
}

uint32_t memshadow_temporal_get_ready(memshadow_temporal_queue_t *queue,
                                       memshadow_temporal_msg_t *out_msgs,
                                       uint32_t max_msgs) {
    if (!queue || !out_msgs) return 0;

    uint32_t ready_count = 0;
    uint32_t write_idx = 0;

    for (uint32_t i = 0; i < queue->count; i++) {
        memshadow_temporal_msg_t *msg = &queue->messages[i];

        if (msg->cancelled) {
            if (msg->payload) free(msg->payload);
            continue;
        }
        if (msg_is_expired(msg)) {
            queue->stats.expired++;
            if (msg->payload) free(msg->payload);
            continue;
        }
        if (msg_is_ready(msg) && ready_count < max_msgs) {
            out_msgs[ready_count] = *msg;
            out_msgs[ready_count].payload = malloc(msg->payload_size);
            if (out_msgs[ready_count].payload) {
                memcpy(out_msgs[ready_count].payload, msg->payload, msg->payload_size);
            }
            ready_count++;
            queue->stats.delivered++;
            if (msg->payload) free(msg->payload);
            continue;
        }

        /* Keep this message in the queue */
        if (write_idx != i) queue->messages[write_idx] = queue->messages[i];
        write_idx++;
    }

    queue->count = write_idx;
    return ready_count;
}

int memshadow_temporal_retry(memshadow_temporal_queue_t *queue,
                              const char *id, uint64_t delay_ms) {
    if (!queue || !id) return -1;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (strcmp(queue->messages[i].id, id) == 0) {
            if (queue->messages[i].retry_count >= queue->messages[i].max_retries)
                return -3;
            queue->messages[i].retry_count++;
            queue->messages[i].deliver_at_ns = memshadow_get_timestamp_ns() + delay_ms * 1000000ULL;
            queue->messages[i].delivered = false;
            queue->stats.retried++;
            return 0;
        }
    }
    return -2;
}

void memshadow_temporal_cleanup(memshadow_temporal_queue_t *queue) {
    if (!queue) return;
    uint32_t write_idx = 0;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (queue->messages[i].cancelled || msg_is_expired(&queue->messages[i])) {
            if (queue->messages[i].payload) free(queue->messages[i].payload);
            queue->stats.expired++;
        } else {
            if (write_idx != i) queue->messages[write_idx] = queue->messages[i];
            write_idx++;
        }
    }
    queue->count = write_idx;
}

uint32_t memshadow_temporal_ready_count(const memshadow_temporal_queue_t *queue) {
    if (!queue) return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (msg_is_ready(&queue->messages[i])) count++;
    }
    return count;
}

uint32_t memshadow_temporal_pending_count(const memshadow_temporal_queue_t *queue) {
    if (!queue) return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < queue->count; i++) {
        if (!queue->messages[i].cancelled && !queue->messages[i].delivered) count++;
    }
    return count;
}

void memshadow_temporal_get_stats(const memshadow_temporal_queue_t *queue,
                                   memshadow_temporal_stats_t *stats) {
    if (queue && stats) *stats = queue->stats;
}
