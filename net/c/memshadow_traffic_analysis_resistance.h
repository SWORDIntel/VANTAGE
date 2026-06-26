/**
 * MEMSHADOW Protocol - Traffic Analysis Resistance Header
 *
 * Implements techniques to resist traffic analysis attacks:
 * - Packet size normalization
 * - Timing pattern masking
 * - Flow correlation resistance
 * - Protocol fingerprint resistance
 * - Dummy traffic generation
 */

#ifndef MEMSHADOW_TRAFFIC_ANALYSIS_RESISTANCE_H
#define MEMSHADOW_TRAFFIC_ANALYSIS_RESISTANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resistance Levels */
typedef enum {
    TRAFFIC_RESISTANCE_NONE = 0,
    TRAFFIC_RESISTANCE_BASIC = 1,
    TRAFFIC_RESISTANCE_ADVANCED = 2,
    TRAFFIC_RESISTANCE_MAXIMUM = 3,
} memshadow_traffic_resistance_level_t;

/* Packet Information for Analysis */
typedef struct {
    size_t size;
    uint64_t timestamp;
    const char *direction; // "inbound" or "outbound"
    const char *protocol;
} memshadow_packet_info_t;

/* Traffic Analysis Resistance Manager */
typedef struct {
    memshadow_traffic_resistance_level_t level;
    size_t *packet_sizes;
    size_t packet_sizes_count;
    size_t packet_sizes_capacity;
    uint64_t *timing_delays;
    size_t timing_delays_count;
    size_t timing_delays_capacity;
    char **flow_ids;
    size_t flow_ids_count;
    size_t flow_ids_capacity;
    uint64_t last_packet_time;
    uint32_t random_seed;
} memshadow_traffic_analysis_resistance_t;

/* Processed Packet Structure */
typedef struct {
    uint8_t **packets;
    size_t packet_count;
    uint64_t *delays;
    size_t delay_count;
} memshadow_processed_packets_t;

/* Function Declarations */

/* Initialize traffic analysis resistance */
int memshadow_traffic_analysis_resistance_init(
    memshadow_traffic_analysis_resistance_t **resistance,
    memshadow_traffic_resistance_level_t level
);

/* Cleanup traffic analysis resistance */
void memshadow_traffic_analysis_resistance_cleanup(
    memshadow_traffic_analysis_resistance_t *resistance
);

/* Process packets with resistance techniques */
int memshadow_traffic_analysis_resistance_process_packets(
    memshadow_traffic_analysis_resistance_t *resistance,
    const uint8_t *const *packets,
    size_t packet_count,
    bool add_dummy_traffic,
    memshadow_processed_packets_t *result
);

/* Initialize processed packets structure */
int memshadow_processed_packets_init(memshadow_processed_packets_t *packets);

/* Cleanup processed packets structure */
void memshadow_processed_packets_cleanup(memshadow_processed_packets_t *packets);

/* Normalize packet size */
int memshadow_traffic_analysis_normalize_packet_size(
    const uint8_t *packet,
    size_t packet_size,
    size_t target_size,
    uint8_t **normalized_packet,
    size_t *normalized_size
);

/* Generate timing delay */
uint64_t memshadow_traffic_analysis_generate_timing_delay(
    memshadow_traffic_analysis_resistance_t *resistance
);

/* Generate dummy packet */
int memshadow_traffic_analysis_generate_dummy_packet(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint8_t **packet,
    size_t *packet_size
);

/* Get flow ID for correlation resistance */
uint32_t memshadow_traffic_analysis_get_flow_id(
    memshadow_traffic_analysis_resistance_t *resistance,
    const char *source,
    const char *destination
);

/* Randomize protocol headers */
int memshadow_traffic_analysis_randomize_headers(
    memshadow_traffic_analysis_resistance_t *resistance,
    char **user_agent,
    uint8_t *ttl,
    uint16_t *window_size
);

/* Add timing noise */
uint64_t memshadow_traffic_analysis_add_timing_noise(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint64_t base_delay
);

/* Defragment packets */
int memshadow_traffic_analysis_defragment_packets(
    const uint8_t *const *packets,
    size_t packet_count,
    uint8_t ***defragmented_packets,
    size_t *defragmented_count
);

/* Apply protocol fingerprint resistance */
int memshadow_traffic_analysis_apply_fingerprint_resistance(
    const uint8_t *packet,
    size_t packet_size,
    uint8_t **modified_packet,
    size_t *modified_size
);

/* Analyze packet patterns */
int memshadow_traffic_analysis_analyze_packet_patterns(
    memshadow_traffic_analysis_resistance_t *resistance,
    const memshadow_packet_info_t *packets,
    size_t packet_count
);

/* Get statistics */
void memshadow_traffic_analysis_get_statistics(
    const memshadow_traffic_analysis_resistance_t *resistance,
    size_t *total_packets_processed,
    size_t *dummy_packets_generated,
    uint64_t *average_delay,
    size_t *unique_flows
);

/* Clean up old packet history */
void memshadow_traffic_analysis_cleanup_old_history(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint64_t max_age_ns
);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_TRAFFIC_ANALYSIS_RESISTANCE_H */