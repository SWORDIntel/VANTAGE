/**
 * MEMSHADOW Protocol - Traffic Analysis Resistance Implementation
 *
 * Implements techniques to resist traffic analysis attacks:
 * - Packet size normalization
 * - Timing pattern masking
 * - Flow correlation resistance
 * - Protocol fingerprint resistance
 * - Dummy traffic generation
 */

#include "memshadow_traffic_analysis_resistance.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Forward declarations for internal functions */
static size_t memshadow_traffic_analysis_get_nearest_standard_size(
    memshadow_traffic_analysis_resistance_t *resistance,
    size_t size
);

/* Default packet sizes for normalization */
static const size_t DEFAULT_PACKET_SIZES[] = {64, 128, 256, 512, 1024, 1280, 1500};

/* Common User-Agents for randomization */
static const char *COMMON_USER_AGENTS[] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "curl/7.68.0",
    "Wget/1.20.3",
    NULL
};

/* Simple random number generator */
static uint32_t simple_rand(uint32_t *seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed & 0x7fffffff;
}

/* Initialize processed packets */
int memshadow_processed_packets_init(memshadow_processed_packets_t *packets) {
    if (!packets) return -1;

    memset(packets, 0, sizeof(memshadow_processed_packets_t));
    return 0;
}

void memshadow_processed_packets_cleanup(memshadow_processed_packets_t *packets) {
    if (!packets) return;

    if (packets->packets) {
        for (size_t i = 0; i < packets->packet_count; i++) {
            free(packets->packets[i]);
        }
        free(packets->packets);
    }

    free(packets->delays);
    memset(packets, 0, sizeof(memshadow_processed_packets_t));
}

/* Traffic Analysis Resistance Functions */
int memshadow_traffic_analysis_resistance_init(
    memshadow_traffic_analysis_resistance_t **resistance,
    memshadow_traffic_resistance_level_t level
) {
    if (!resistance) return -1;

    *resistance = calloc(1, sizeof(memshadow_traffic_analysis_resistance_t));
    if (!*resistance) return -1;

    (*resistance)->level = level;
    (*resistance)->random_seed = (uint32_t)time(NULL);

    // Initialize packet sizes array
    (*resistance)->packet_sizes_capacity = 16;
    (*resistance)->packet_sizes = malloc(sizeof(size_t) * (*resistance)->packet_sizes_capacity);
    if (!(*resistance)->packet_sizes) {
        free(*resistance);
        return -1;
    }

    // Copy default packet sizes
    size_t default_count = sizeof(DEFAULT_PACKET_SIZES) / sizeof(DEFAULT_PACKET_SIZES[0]);
    for (size_t i = 0; i < default_count && i < (*resistance)->packet_sizes_capacity; i++) {
        (*resistance)->packet_sizes[i] = DEFAULT_PACKET_SIZES[i];
    }
    (*resistance)->packet_sizes_count = default_count;

    // Initialize timing delays array
    (*resistance)->timing_delays_capacity = 1000;
    (*resistance)->timing_delays = malloc(sizeof(uint64_t) * (*resistance)->timing_delays_capacity);
    if (!(*resistance)->timing_delays) {
        free((*resistance)->packet_sizes);
        free(*resistance);
        return -1;
    }

    // Initialize flow IDs array
    (*resistance)->flow_ids_capacity = 100;
    (*resistance)->flow_ids = malloc(sizeof(char*) * (*resistance)->flow_ids_capacity);
    if (!(*resistance)->flow_ids) {
        free((*resistance)->packet_sizes);
        free((*resistance)->timing_delays);
        free(*resistance);
        return -1;
    }

    return 0;
}

void memshadow_traffic_analysis_resistance_cleanup(
    memshadow_traffic_analysis_resistance_t *resistance
) {
    if (!resistance) return;

    free(resistance->packet_sizes);
    free(resistance->timing_delays);

    if (resistance->flow_ids) {
        for (size_t i = 0; i < resistance->flow_ids_count; i++) {
            free(resistance->flow_ids[i]);
        }
        free(resistance->flow_ids);
    }

    memset(resistance, 0, sizeof(memshadow_traffic_analysis_resistance_t));
    free(resistance);
}

int memshadow_traffic_analysis_resistance_process_packets(
    memshadow_traffic_analysis_resistance_t *resistance,
    const uint8_t *const *packets,
    size_t packet_count,
    bool add_dummy_traffic,
    memshadow_processed_packets_t *result
) {
    if (!resistance || !packets || !result) return -1;

    memshadow_processed_packets_init(result);

    // Allocate result arrays
    result->packets = malloc(sizeof(uint8_t*) * packet_count * 2); // *2 for potential dummy packets
    result->delays = malloc(sizeof(uint64_t) * packet_count * 2);

    if (!result->packets || !result->delays) {
        memshadow_processed_packets_cleanup(result);
        return -1;
    }

    result->packet_count = 0;
    result->delay_count = 0;

    for (size_t i = 0; i < packet_count; i++) {
        const uint8_t *packet = packets[i];
        size_t packet_size = strlen((const char*)packet); // Simplified

        // Normalize packet size if resistance level allows
        uint8_t *normalized_packet = NULL;
        size_t normalized_size = packet_size;

        if (resistance->level != TRAFFIC_RESISTANCE_NONE) {
            size_t target_size = memshadow_traffic_analysis_get_nearest_standard_size(resistance, packet_size);
            if (memshadow_traffic_analysis_normalize_packet_size(packet, packet_size, target_size,
                &normalized_packet, &normalized_size) != 0) {
                normalized_packet = malloc(packet_size);
                if (normalized_packet) {
                    memcpy(normalized_packet, packet, packet_size);
                    normalized_size = packet_size;
                }
            }
        } else {
            normalized_packet = malloc(packet_size);
            if (normalized_packet) {
                memcpy(normalized_packet, packet, packet_size);
                normalized_size = packet_size;
            }
        }

        if (normalized_packet) {
            result->packets[result->packet_count++] = normalized_packet;
            result->delays[result->delay_count++] = memshadow_traffic_analysis_generate_timing_delay(resistance);

            // Add dummy traffic if requested
            if (add_dummy_traffic && resistance->level != TRAFFIC_RESISTANCE_NONE) {
                uint32_t rand_val = simple_rand(&resistance->random_seed);
                if ((rand_val % 100) < 15) { // 15% chance of dummy traffic
                    uint8_t *dummy_packet = NULL;
                    size_t dummy_size = 0;

                    if (memshadow_traffic_analysis_generate_dummy_packet(resistance, &dummy_packet, &dummy_size) == 0) {
                        if (result->packet_count < packet_count * 2) {
                            result->packets[result->packet_count++] = dummy_packet;
                            result->delays[result->delay_count++] = memshadow_traffic_analysis_generate_timing_delay(resistance);
                        } else {
                            free(dummy_packet);
                        }
                    }
                }
            }
        }
    }

    return 0;
}

int memshadow_traffic_analysis_normalize_packet_size(
    const uint8_t *packet,
    size_t packet_size,
    size_t target_size,
    uint8_t **normalized_packet,
    size_t *normalized_size
) {
    if (!packet || !normalized_packet || !normalized_size) return -1;

    *normalized_size = target_size;
    *normalized_packet = malloc(target_size);

    if (!*normalized_packet) return -1;

    if (packet_size < target_size) {
        // Pad packet
        memcpy(*normalized_packet, packet, packet_size);
        memset(*normalized_packet + packet_size, 0, target_size - packet_size);
    } else {
        // Truncate packet
        memcpy(*normalized_packet, packet, target_size);
    }

    return 0;
}

size_t memshadow_traffic_analysis_get_nearest_standard_size(
    memshadow_traffic_analysis_resistance_t *resistance,
    size_t size
) {
    if (!resistance || resistance->packet_sizes_count == 0) {
        return size;
    }

    size_t nearest = resistance->packet_sizes[0];
    size_t min_diff = (size_t)abs((int)nearest - (int)size);

    for (size_t i = 1; i < resistance->packet_sizes_count; i++) {
        size_t diff = (size_t)abs((int)resistance->packet_sizes[i] - (int)size);
        if (diff < min_diff) {
            min_diff = diff;
            nearest = resistance->packet_sizes[i];
        }
    }

    return nearest;
}

uint64_t memshadow_traffic_analysis_generate_timing_delay(
    memshadow_traffic_analysis_resistance_t *resistance
) {
    if (!resistance) return 0;

    uint32_t rand_val = simple_rand(&resistance->random_seed);

    switch (resistance->level) {
        case TRAFFIC_RESISTANCE_NONE:
            return 0;
        case TRAFFIC_RESISTANCE_BASIC:
            return (rand_val % 100) + 10; // 10-110ms
        case TRAFFIC_RESISTANCE_ADVANCED:
            return (rand_val % 500) + 50; // 50-550ms
        case TRAFFIC_RESISTANCE_MAXIMUM:
            return (rand_val % 1000) + 100; // 100-1100ms
        default:
            return 0;
    }
}

int memshadow_traffic_analysis_generate_dummy_packet(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint8_t **packet,
    size_t *packet_size
) {
    if (!resistance || !packet || !packet_size) return -1;

    uint32_t rand_val = simple_rand(&resistance->random_seed);
    size_t size;

    switch (resistance->level) {
        case TRAFFIC_RESISTANCE_BASIC:
            size = 64;
            break;
        case TRAFFIC_RESISTANCE_ADVANCED:
            size = 128 + (rand_val % 128); // 128-256
            break;
        case TRAFFIC_RESISTANCE_MAXIMUM:
            size = 256 + (rand_val % 256); // 256-512
            break;
        default:
            size = 64;
    }

    *packet_size = size;
    *packet = malloc(size);

    if (!*packet) return -1;

    // Fill with random-looking data
    for (size_t i = 0; i < size; i++) {
        (*packet)[i] = (uint8_t)(rand_val + i) % 256;
    }

    // Add some protocol-like patterns to make it look legitimate
    if (size >= 4) {
        // Fake TCP/IP header pattern
        (*packet)[0] = 0x45; // IP version 4, header length 5
        (*packet)[1] = 0x00; // DSCP/ECN
        (*packet)[9] = 6;    // Protocol (TCP)
    }

    return 0;
}

uint32_t memshadow_traffic_analysis_get_flow_id(
    memshadow_traffic_analysis_resistance_t *resistance,
    const char *source,
    const char *destination
) {
    if (!resistance || !source || !destination) return 0;

    char flow_key[256];
    snprintf(flow_key, sizeof(flow_key), "%s->%s", source, destination);

    // Simple hash-based flow ID generation
    uint32_t hash = 0;
    for (size_t i = 0; flow_key[i]; i++) {
        hash = hash * 31 + (uint32_t)flow_key[i];
    }

    return hash;
}

int memshadow_traffic_analysis_randomize_headers(
    memshadow_traffic_analysis_resistance_t *resistance,
    char **user_agent,
    uint8_t *ttl,
    uint16_t *window_size
) {
    if (!resistance) return -1;

    uint32_t rand_val = simple_rand(&resistance->random_seed);

    // Select random user agent
    if (user_agent) {
        int ua_count = 0;
        while (COMMON_USER_AGENTS[ua_count]) ua_count++;

        int ua_index = rand_val % ua_count;
        size_t ua_len = strlen(COMMON_USER_AGENTS[ua_index]) + 1;
        *user_agent = malloc(ua_len);

        if (!*user_agent) return -1;
        strcpy(*user_agent, COMMON_USER_AGENTS[ua_index]);
    }

    // Randomize TTL
    if (ttl) {
        *ttl = 32 + (rand_val % 224); // 32-255
    }

    // Randomize window size
    if (window_size) {
        *window_size = 1024 + (rand_val % (65535 - 1024)); // 1024-65535
    }

    return 0;
}

uint64_t memshadow_traffic_analysis_add_timing_noise(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint64_t base_delay
) {
    if (!resistance || resistance->level == TRAFFIC_RESISTANCE_NONE) {
        return base_delay;
    }

    uint32_t rand_val = simple_rand(&resistance->random_seed);
    double noise_factor;

    switch (resistance->level) {
        case TRAFFIC_RESISTANCE_BASIC:
            noise_factor = 0.1; // ±10% noise
            break;
        case TRAFFIC_RESISTANCE_ADVANCED:
            noise_factor = 0.25; // ±25% noise
            break;
        case TRAFFIC_RESISTANCE_MAXIMUM:
            noise_factor = 0.5; // ±50% noise
            break;
        default:
            return base_delay;
    }

    uint64_t noise_range = (uint64_t)(base_delay * noise_factor);
    if (noise_range == 0) noise_range = 1;

    int64_t noise = (int64_t)(rand_val % (2 * noise_range)) - (int64_t)noise_range;
    int64_t noisy_delay = (int64_t)base_delay + noise;

    return (uint64_t)(noisy_delay > 0 ? noisy_delay : 0);
}

int memshadow_traffic_analysis_defragment_packets(
    const uint8_t *const *packets,
    size_t packet_count,
    uint8_t ***defragmented_packets,
    size_t *defragmented_count
) {
    if (!packets || !defragmented_packets || !defragmented_count) return -1;

    // Simple defragmentation: combine small packets
    *defragmented_packets = NULL;
    *defragmented_count = 0;

    if (packet_count == 0) return 0;

    // Allocate result array
    *defragmented_packets = malloc(sizeof(uint8_t*) * packet_count);
    if (!*defragmented_packets) return -1;

    uint8_t *current_fragment = NULL;
    size_t current_size = 0;
    size_t fragment_count = 0;

    for (size_t i = 0; i < packet_count; i++) {
        size_t packet_size = strlen((const char*)packets[i]);

        if (current_fragment == NULL) {
            // Start new fragment
            current_fragment = malloc(packet_size);
            if (!current_fragment) {
                // Cleanup and return error
                for (size_t j = 0; j < fragment_count; j++) {
                    free((*defragmented_packets)[j]);
                }
                free(*defragmented_packets);
                return -1;
            }
            memcpy(current_fragment, packets[i], packet_size);
            current_size = packet_size;
        } else if (current_size + packet_size <= 1400) { // MTU-ish
            // Add to current fragment
            uint8_t *new_fragment = realloc(current_fragment, current_size + packet_size);
            if (!new_fragment) {
                free(current_fragment);
                for (size_t j = 0; j < fragment_count; j++) {
                    free((*defragmented_packets)[j]);
                }
                free(*defragmented_packets);
                return -1;
            }
            current_fragment = new_fragment;
            memcpy(current_fragment + current_size, packets[i], packet_size);
            current_size += packet_size;
        } else {
            // Save current fragment and start new one
            (*defragmented_packets)[fragment_count++] = current_fragment;
            current_fragment = malloc(packet_size);
            if (!current_fragment) {
                for (size_t j = 0; j < fragment_count; j++) {
                    free((*defragmented_packets)[j]);
                }
                free(*defragmented_packets);
                return -1;
            }
            memcpy(current_fragment, packets[i], packet_size);
            current_size = packet_size;
        }
    }

    // Save final fragment
    if (current_fragment) {
        (*defragmented_packets)[fragment_count++] = current_fragment;
    }

    *defragmented_count = fragment_count;
    return 0;
}

int memshadow_traffic_analysis_apply_fingerprint_resistance(
    const uint8_t *packet,
    size_t packet_size,
    uint8_t **modified_packet,
    size_t *modified_size
) {
    if (!packet || !modified_packet || !modified_size) return -1;

    // Add HTTP-like headers to non-HTTP traffic
    const char *fake_headers = "X-Memshadow: true\r\n";
    size_t headers_len = strlen(fake_headers);

    if (packet_size + headers_len > 1500) {
        // Packet too big, just copy
        *modified_size = packet_size;
        *modified_packet = malloc(packet_size);
        if (!*modified_packet) return -1;
        memcpy(*modified_packet, packet, packet_size);
        return 0;
    }

    // Check if it already looks like HTTP
    bool looks_like_http = (packet_size >= 4 &&
        (memcmp(packet, "GET ", 4) == 0 ||
         memcmp(packet, "POST ", 5) == 0 ||
         memcmp(packet, "HTTP/", 5) == 0));

    if (looks_like_http) {
        // Already looks like HTTP, just copy
        *modified_size = packet_size;
        *modified_packet = malloc(packet_size);
        if (!*modified_packet) return -1;
        memcpy(*modified_packet, packet, packet_size);
        return 0;
    }

    // Add fake headers
    *modified_size = headers_len + packet_size;
    *modified_packet = malloc(*modified_size);
    if (!*modified_packet) return -1;

    memcpy(*modified_packet, fake_headers, headers_len);
    memcpy(*modified_packet + headers_len, packet, packet_size);

    return 0;
}

int memshadow_traffic_analysis_analyze_packet_patterns(
    memshadow_traffic_analysis_resistance_t *resistance,
    const memshadow_packet_info_t *packets,
    size_t packet_count
) {
    if (!resistance || !packets) return -1;

    // Simple analysis: count packets by size
    size_t size_counts[1500] = {0}; // Up to MTU
    size_t max_count_size = 0;
    size_t max_count = 0;

    for (size_t i = 0; i < packet_count; i++) {
        if (packets[i].size < sizeof(size_counts)/sizeof(size_counts[0])) {
            size_counts[packets[i].size]++;
            if (size_counts[packets[i].size] > max_count) {
                max_count = size_counts[packets[i].size];
                max_count_size = packets[i].size;
            }
        }
    }

    // Update preferred packet sizes based on analysis
    if (max_count > 5 && resistance->packet_sizes_count < resistance->packet_sizes_capacity) {
        resistance->packet_sizes[resistance->packet_sizes_count++] = max_count_size;
    }

    return 0;
}

void memshadow_traffic_analysis_get_statistics(
    const memshadow_traffic_analysis_resistance_t *resistance,
    size_t *total_packets_processed,
    size_t *dummy_packets_generated,
    uint64_t *average_delay,
    size_t *unique_flows
) {
    if (total_packets_processed) *total_packets_processed = 0; // Not tracked
    if (dummy_packets_generated) *dummy_packets_generated = 0; // Not tracked

    if (average_delay && resistance->timing_delays_count > 0) {
        uint64_t sum = 0;
        for (size_t i = 0; i < resistance->timing_delays_count; i++) {
            sum += resistance->timing_delays[i];
        }
        *average_delay = sum / resistance->timing_delays_count;
    } else if (average_delay) {
        *average_delay = 0;
    }

    if (unique_flows) *unique_flows = resistance->flow_ids_count;
}

void memshadow_traffic_analysis_cleanup_old_history(
    memshadow_traffic_analysis_resistance_t *resistance,
    uint64_t max_age_ns
) {
    if (!resistance) return;

    // Simplified: just reset counters (in real implementation, would check timestamps)
    resistance->timing_delays_count = 0;
    resistance->flow_ids_count = 0;
}