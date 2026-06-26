/*
 * MEMSHADOW Protocol v3 Header
 */

#ifndef MEMSHADOW_PROTOCOL_H
#define MEMSHADOW_PROTOCOL_H

#include <windows.h>
#include <stdbool.h>

#define MEMSHADOW_MAGIC 0x4D53485700000000ULL
#define MEMSHADOW_VERSION 3
#define MEMSHADOW_HEADER_SIZE 32

// Message types
#define MSG_APP_REGISTER 0x2101
#define MSG_APP_REGISTER_ACK 0x2102
#define MSG_APP_COMMAND 0x2103
#define MSG_APP_COMMAND_ACK 0x2104
#define MSG_APP_TELEMETRY 0x2105
#define MSG_APP_HEARTBEAT 0x2106
#define MSG_APP_ERROR 0x2107

// Header flags
#define FLAG_REQUIRES_ACK 0x01
#define FLAG_PQC_SIGNED 0x02
#define FLAG_HMAC_PRESENT 0x04

typedef struct {
    uint64_t magic;
    uint16_t version;
    uint16_t priority;
    uint16_t msg_type;
    uint16_t flags_batch;
    uint32_t payload_len;
    uint32_t sequence_num;
    uint64_t timestamp_ns;
} MemshadowHeader;

bool memshadow_header_pack(uint16_t priority, uint8_t flags, uint16_t msg_type,
                          uint8_t batch_count, uint32_t payload_len,
                          uint32_t sequence_num, uint8_t *output);
bool memshadow_header_unpack(const uint8_t *input, MemshadowHeader *header);
bool memshadow_compute_hmac_sha384(const uint8_t *data, size_t data_len,
                                   const uint8_t *key, size_t key_len,
                                   uint8_t *hmac_out);
bool memshadow_verify_hmac(const uint8_t *data, size_t data_len,
                          const uint8_t *key, size_t key_len,
                          const uint8_t *expected_hmac);
bool memshadow_pack_app_register(const uint8_t *app_id, const char *name,
                                 const char *capabilities_json,
                                 const uint8_t *session_token,
                                 uint8_t **payload, size_t *payload_len);
bool memshadow_pack_app_command(const uint8_t *app_id, uint64_t command_id,
                                uint16_t cmd_type, const uint8_t *args, size_t args_len,
                                uint32_t ttl_ms, const uint8_t *session_token,
                                uint8_t **payload, size_t *payload_len);
bool memshadow_unpack_app_command(const uint8_t *payload, size_t payload_len,
                                  uint8_t *app_id, uint64_t *command_id,
                                  uint16_t *cmd_type, uint32_t *ttl_ms,
                                  uint8_t **args, size_t *args_len);
bool memshadow_pack_app_command_ack(const uint8_t *app_id, uint64_t command_id,
                                    uint8_t status, const uint8_t *result, size_t result_len,
                                    const uint8_t *session_token,
                                    uint8_t **payload, size_t *payload_len);
bool memshadow_create_message(uint16_t priority, uint8_t flags, uint16_t msg_type,
                              uint8_t batch_count, const uint8_t *payload, size_t payload_len,
                              uint32_t sequence_num,
                              uint8_t **message, size_t *message_len);
bool memshadow_parse_message(const uint8_t *message, size_t message_len,
                             MemshadowHeader *header,
                             uint8_t **payload, size_t *payload_len);

#endif // MEMSHADOW_PROTOCOL_H
