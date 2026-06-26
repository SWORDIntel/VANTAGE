/**
 * MEMSHADOW Protocol v3.0 - Stealth Mode (C)
 *
 * Unified stealth: DPI evasion, traffic shaping, protocol mimicry,
 * dummy traffic, flow rotation.
 */

#ifndef MEMSHADOW_STEALTH_H
#define MEMSHADOW_STEALTH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STEALTH_NONE = 0,
    STEALTH_LOW = 1,
    STEALTH_MEDIUM = 2,
    STEALTH_HIGH = 3,
    STEALTH_MAXIMUM = 4,
} memshadow_stealth_level_t;

typedef enum {
    MIMIC_NONE = 0,
    MIMIC_HTTP = 1,
    MIMIC_HTTPS = 2,
    MIMIC_DNS = 3,
    MIMIC_SMTP = 4,
    MIMIC_SSH = 5,
    MIMIC_WEBSOCKET = 6,
} memshadow_mimic_protocol_t;

typedef struct {
    memshadow_stealth_level_t level;
    memshadow_mimic_protocol_t mimic_protocol;
    uint16_t padding_min;
    uint16_t padding_max;
    uint32_t jitter_base_ms;
    uint32_t jitter_range_ms;
    double   dummy_traffic_rate;
    uint32_t flow_rotation_interval_ms;
    bool     normalize_packet_sizes;
    bool     randomize_headers;
} memshadow_stealth_config_t;

typedef struct {
    uint64_t packets_sent;
    uint64_t dummy_packets_sent;
    uint64_t bytes_padded;
    uint64_t flow_id;
    memshadow_stealth_level_t level;
} memshadow_stealth_stats_t;

typedef struct {
    memshadow_stealth_config_t config;
    uint64_t flow_id;
    uint64_t last_flow_rotation_ns;
    memshadow_stealth_stats_t stats;
} memshadow_stealth_engine_t;

void memshadow_stealth_init(memshadow_stealth_engine_t *engine, memshadow_stealth_level_t level);
void memshadow_stealth_init_config(memshadow_stealth_engine_t *engine, const memshadow_stealth_config_t *config);

/* Apply stealth to outgoing data. Returns allocated buffer (caller must free). */
uint8_t *memshadow_stealth_apply(memshadow_stealth_engine_t *engine,
                                  const uint8_t *data, size_t data_size,
                                  size_t *out_size);

/* Strip stealth from incoming data. Returns allocated buffer (caller must free). */
uint8_t *memshadow_stealth_strip(const memshadow_stealth_engine_t *engine,
                                  const uint8_t *data, size_t data_size,
                                  size_t *out_size);

/* Generate dummy traffic packet. Returns allocated buffer (caller must free). */
uint8_t *memshadow_stealth_generate_dummy(memshadow_stealth_engine_t *engine, size_t *out_size);

/* Check if dummy packet should be sent */
bool memshadow_stealth_should_send_dummy(const memshadow_stealth_engine_t *engine);

/* Get jitter delay in milliseconds */
uint32_t memshadow_stealth_get_jitter_ms(const memshadow_stealth_engine_t *engine);

/* Get stats */
void memshadow_stealth_get_stats(const memshadow_stealth_engine_t *engine, memshadow_stealth_stats_t *stats);

/* Get default config for a stealth level */
memshadow_stealth_config_t memshadow_stealth_default_config(memshadow_stealth_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_STEALTH_H */
