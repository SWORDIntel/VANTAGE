/**
 * MEMSHADOW Protocol v3.0 - Stealth Mode Implementation (C)
 */

#include "memshadow_stealth.h"
#include "memshadow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/random.h>
#endif

static int stealth_random(uint8_t *buf, size_t len) {
#ifdef __linux__
    return (getrandom(buf, len, 0) == (ssize_t)len) ? 0 : -1;
#else
    for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)(rand() & 0xFF);
    return 0;
#endif
}

static const size_t NORMALIZED_SIZES[] = {64, 128, 256, 512, 576, 1024, 1280, 1400, 1460, 1500};
static const size_t NORMALIZED_COUNT = sizeof(NORMALIZED_SIZES) / sizeof(NORMALIZED_SIZES[0]);

memshadow_stealth_config_t memshadow_stealth_default_config(memshadow_stealth_level_t level) {
    memshadow_stealth_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.level = level;

    switch (level) {
        case STEALTH_NONE:
            break;
        case STEALTH_LOW:
            cfg.padding_min = 8; cfg.padding_max = 64;
            cfg.jitter_base_ms = 50; cfg.jitter_range_ms = 25;
            cfg.flow_rotation_interval_ms = 300000;
            break;
        case STEALTH_MEDIUM:
            cfg.mimic_protocol = MIMIC_HTTPS;
            cfg.padding_min = 16; cfg.padding_max = 256;
            cfg.jitter_base_ms = 100; cfg.jitter_range_ms = 50;
            cfg.dummy_traffic_rate = 0.10;
            cfg.flow_rotation_interval_ms = 60000;
            cfg.normalize_packet_sizes = true;
            cfg.randomize_headers = true;
            break;
        case STEALTH_HIGH:
            cfg.mimic_protocol = MIMIC_HTTPS;
            cfg.padding_min = 32; cfg.padding_max = 512;
            cfg.jitter_base_ms = 100; cfg.jitter_range_ms = 50;
            cfg.dummy_traffic_rate = 0.10;
            cfg.flow_rotation_interval_ms = 60000;
            cfg.normalize_packet_sizes = true;
            cfg.randomize_headers = true;
            break;
        case STEALTH_MAXIMUM:
            cfg.mimic_protocol = MIMIC_HTTPS;
            cfg.padding_min = 64; cfg.padding_max = 1024;
            cfg.jitter_base_ms = 200; cfg.jitter_range_ms = 100;
            cfg.dummy_traffic_rate = 0.15;
            cfg.flow_rotation_interval_ms = 30000;
            cfg.normalize_packet_sizes = true;
            cfg.randomize_headers = true;
            break;
    }
    return cfg;
}

void memshadow_stealth_init(memshadow_stealth_engine_t *engine, memshadow_stealth_level_t level) {
    memshadow_stealth_config_t cfg = memshadow_stealth_default_config(level);
    memshadow_stealth_init_config(engine, &cfg);
}

void memshadow_stealth_init_config(memshadow_stealth_engine_t *engine, const memshadow_stealth_config_t *config) {
    memset(engine, 0, sizeof(*engine));
    engine->config = *config;
    uint8_t flow_bytes[8];
    stealth_random(flow_bytes, 8);
    engine->flow_id = 0;
    for (int i = 0; i < 8; i++) engine->flow_id = (engine->flow_id << 8) | flow_bytes[i];
    engine->last_flow_rotation_ns = memshadow_get_timestamp_ns();
    engine->stats.level = config->level;
    engine->stats.flow_id = engine->flow_id;
}

static size_t find_normalized_size(size_t data_len) {
    for (size_t i = 0; i < NORMALIZED_COUNT; i++) {
        if (NORMALIZED_SIZES[i] >= data_len) return NORMALIZED_SIZES[i];
    }
    return 1500;
}

static void maybe_rotate_flow(memshadow_stealth_engine_t *engine) {
    if (engine->config.flow_rotation_interval_ms == 0) return;
    uint64_t now = memshadow_get_timestamp_ns();
    uint64_t elapsed_ms = (now - engine->last_flow_rotation_ns) / 1000000ULL;
    if (elapsed_ms >= engine->config.flow_rotation_interval_ms) {
        uint8_t new_flow[8];
        stealth_random(new_flow, 8);
        engine->flow_id = 0;
        for (int i = 0; i < 8; i++) engine->flow_id = (engine->flow_id << 8) | new_flow[i];
        engine->last_flow_rotation_ns = now;
        engine->stats.flow_id = engine->flow_id;
    }
}

static uint8_t *wrap_tls_record(const uint8_t *data, size_t data_size, size_t *out_size) {
    *out_size = 5 + data_size;
    uint8_t *buf = malloc(*out_size);
    if (!buf) return NULL;
    buf[0] = 0x17; /* Application Data */
    buf[1] = 0x03; buf[2] = 0x03; /* TLS 1.2 */
    buf[3] = (data_size >> 8) & 0xFF;
    buf[4] = data_size & 0xFF;
    memcpy(buf + 5, data, data_size);
    return buf;
}

static uint8_t *unwrap_tls_record(const uint8_t *data, size_t data_size, size_t *out_size) {
    if (data_size > 5 && data[0] == 0x17) {
        uint16_t payload_len = ((uint16_t)data[3] << 8) | data[4];
        if (data_size >= (size_t)(5 + payload_len)) {
            *out_size = payload_len;
            uint8_t *buf = malloc(payload_len);
            if (buf) memcpy(buf, data + 5, payload_len);
            return buf;
        }
    }
    *out_size = data_size;
    uint8_t *buf = malloc(data_size);
    if (buf) memcpy(buf, data, data_size);
    return buf;
}

uint8_t *memshadow_stealth_apply(memshadow_stealth_engine_t *engine,
                                  const uint8_t *data, size_t data_size,
                                  size_t *out_size) {
    if (!engine || !data || !out_size) return NULL;
    if (engine->config.level == STEALTH_NONE) {
        *out_size = data_size;
        uint8_t *buf = malloc(data_size);
        if (buf) memcpy(buf, data, data_size);
        return buf;
    }

    /* 1. Pad to normalized size (prepend 2-byte original length) */
    size_t target_size;
    if (engine->config.normalize_packet_sizes) {
        target_size = find_normalized_size(data_size + 2);
    } else {
        uint8_t rng[2];
        stealth_random(rng, 2);
        uint16_t range = engine->config.padding_max - engine->config.padding_min;
        uint16_t pad = engine->config.padding_min + (((uint16_t)rng[0] << 8 | rng[1]) % (range + 1));
        target_size = data_size + 2 + pad;
    }

    uint8_t *padded = malloc(target_size);
    if (!padded) return NULL;
    padded[0] = (data_size >> 8) & 0xFF;
    padded[1] = data_size & 0xFF;
    memcpy(padded + 2, data, data_size);
    if (target_size > data_size + 2) {
        stealth_random(padded + 2 + data_size, target_size - data_size - 2);
    }
    engine->stats.bytes_padded += target_size - data_size - 2;

    /* 2. Protocol mimicry */
    uint8_t *wrapped = padded;
    size_t wrapped_size = target_size;
    if (engine->config.mimic_protocol == MIMIC_HTTPS || engine->config.mimic_protocol == MIMIC_WEBSOCKET) {
        wrapped = wrap_tls_record(padded, target_size, &wrapped_size);
        free(padded);
        if (!wrapped) return NULL;
    }

    /* 3. Flow rotation check */
    maybe_rotate_flow(engine);

    engine->stats.packets_sent++;
    *out_size = wrapped_size;
    return wrapped;
}

uint8_t *memshadow_stealth_strip(const memshadow_stealth_engine_t *engine,
                                  const uint8_t *data, size_t data_size,
                                  size_t *out_size) {
    if (!engine || !data || !out_size) return NULL;
    if (engine->config.level == STEALTH_NONE) {
        *out_size = data_size;
        uint8_t *buf = malloc(data_size);
        if (buf) memcpy(buf, data, data_size);
        return buf;
    }

    /* 1. Unwrap protocol */
    uint8_t *unwrapped;
    size_t unwrapped_size;
    if (engine->config.mimic_protocol == MIMIC_HTTPS || engine->config.mimic_protocol == MIMIC_WEBSOCKET) {
        unwrapped = unwrap_tls_record(data, data_size, &unwrapped_size);
    } else {
        unwrapped_size = data_size;
        unwrapped = malloc(data_size);
        if (unwrapped) memcpy(unwrapped, data, data_size);
    }
    if (!unwrapped) return NULL;

    /* 2. Remove padding (read 2-byte original length) */
    if (unwrapped_size < 2) {
        *out_size = unwrapped_size;
        return unwrapped;
    }
    uint16_t original_len = ((uint16_t)unwrapped[0] << 8) | unwrapped[1];
    if (original_len + 2 <= unwrapped_size) {
        *out_size = original_len;
        uint8_t *result = malloc(original_len);
        if (result) memcpy(result, unwrapped + 2, original_len);
        free(unwrapped);
        return result;
    }

    *out_size = unwrapped_size;
    return unwrapped;
}

uint8_t *memshadow_stealth_generate_dummy(memshadow_stealth_engine_t *engine, size_t *out_size) {
    if (!engine || !out_size) return NULL;
    size_t idx = engine->stats.packets_sent % NORMALIZED_COUNT;
    size_t size = NORMALIZED_SIZES[idx];
    uint8_t *dummy = malloc(size);
    if (!dummy) return NULL;
    stealth_random(dummy, size);
    dummy[0] = 0xFF; /* Dummy marker */
    engine->stats.dummy_packets_sent++;
    return memshadow_stealth_apply(engine, dummy, size, out_size);
}

bool memshadow_stealth_should_send_dummy(const memshadow_stealth_engine_t *engine) {
    if (!engine || engine->config.dummy_traffic_rate <= 0.0) return false;
    uint8_t rng;
    stealth_random(&rng, 1);
    return ((double)rng / 255.0) < engine->config.dummy_traffic_rate;
}

uint32_t memshadow_stealth_get_jitter_ms(const memshadow_stealth_engine_t *engine) {
    if (!engine || engine->config.jitter_base_ms == 0) return 0;
    uint8_t rng[2];
    stealth_random(rng, 2);
    uint16_t offset = ((uint16_t)rng[0] << 8 | rng[1]) % (engine->config.jitter_range_ms * 2 + 1);
    return engine->config.jitter_base_ms + offset - engine->config.jitter_range_ms;
}

void memshadow_stealth_get_stats(const memshadow_stealth_engine_t *engine, memshadow_stealth_stats_t *stats) {
    if (engine && stats) *stats = engine->stats;
}
