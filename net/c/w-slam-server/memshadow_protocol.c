/*
 * MEMSHADOW Protocol v3 Implementation - C Version
 * 
 * Complete implementation of MEMSHADOW v3 protocol:
 * - 32-byte header format
 * - APP_REGISTER, APP_COMMAND, APP_COMMAND_ACK messages
 * - HMAC-SHA384 authentication (CNSA 2.0 compliant)
 * - Big-endian byte order
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#pragma comment(lib, "advapi32.lib")

// MEMSHADOW constants
#define MEMSHADOW_MAGIC 0x4D53485700000000ULL  // "MSHW" + padding
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
#define MSG_APP_BULK_COMMAND 0x2108

// Header flags
#define FLAG_REQUIRES_ACK 0x01
#define FLAG_PQC_SIGNED 0x02
#define FLAG_HMAC_PRESENT 0x04
#define FLAG_ORIGINATOR_NODE 0x08

// MEMSHADOW header structure
typedef struct {
    uint64_t magic;
    uint16_t version;
    uint16_t priority;
    uint16_t msg_type;
    uint16_t flags_batch;  // Flags (low byte) + batch_count (high byte)
    uint32_t payload_len;
    uint32_t sequence_num;
    uint64_t timestamp_ns;
} MemshadowHeader;


/*
 * Get current timestamp in nanoseconds
 */
static uint64_t get_timestamp_ns(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    // Convert to nanoseconds (FILETIME is in 100ns intervals since 1601)
    uint64_t time_100ns = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    
    // Convert to Unix epoch nanoseconds
    uint64_t unix_epoch_100ns = 116444736000000000ULL;
    uint64_t unix_time_100ns = time_100ns - unix_epoch_100ns;
    
    return unix_time_100ns * 100;  // Convert to nanoseconds
}


/*
 * Pack MEMSHADOW v3 Header (32 bytes, big-endian)
 */
bool memshadow_header_pack(uint16_t priority, uint8_t flags, uint16_t msg_type,
                           uint8_t batch_count, uint32_t payload_len,
                           uint32_t sequence_num, uint8_t *output) {
    if (!output) {
        return false;
    }
    
    uint64_t timestamp_ns = get_timestamp_ns();
    uint16_t flags_batch = (flags & 0xFF) | ((batch_count & 0xFF) << 8);
    
    // Pack in big-endian format
    // magic (8 bytes)
    output[0] = (MEMSHADOW_MAGIC >> 56) & 0xFF;
    output[1] = (MEMSHADOW_MAGIC >> 48) & 0xFF;
    output[2] = (MEMSHADOW_MAGIC >> 40) & 0xFF;
    output[3] = (MEMSHADOW_MAGIC >> 32) & 0xFF;
    output[4] = (MEMSHADOW_MAGIC >> 24) & 0xFF;
    output[5] = (MEMSHADOW_MAGIC >> 16) & 0xFF;
    output[6] = (MEMSHADOW_MAGIC >> 8) & 0xFF;
    output[7] = MEMSHADOW_MAGIC & 0xFF;
    
    // version (2 bytes)
    output[8] = (MEMSHADOW_VERSION >> 8) & 0xFF;
    output[9] = MEMSHADOW_VERSION & 0xFF;
    
    // priority (2 bytes)
    output[10] = (priority >> 8) & 0xFF;
    output[11] = priority & 0xFF;
    
    // msg_type (2 bytes)
    output[12] = (msg_type >> 8) & 0xFF;
    output[13] = msg_type & 0xFF;
    
    // flags_batch (2 bytes)
    output[14] = (flags_batch >> 8) & 0xFF;
    output[15] = flags_batch & 0xFF;
    
    // payload_len (4 bytes)
    output[16] = (payload_len >> 24) & 0xFF;
    output[17] = (payload_len >> 16) & 0xFF;
    output[18] = (payload_len >> 8) & 0xFF;
    output[19] = payload_len & 0xFF;
    
    // sequence_num (4 bytes)
    output[20] = (sequence_num >> 24) & 0xFF;
    output[21] = (sequence_num >> 16) & 0xFF;
    output[22] = (sequence_num >> 8) & 0xFF;
    output[23] = sequence_num & 0xFF;
    
    // timestamp_ns (8 bytes)
    output[24] = (timestamp_ns >> 56) & 0xFF;
    output[25] = (timestamp_ns >> 48) & 0xFF;
    output[26] = (timestamp_ns >> 40) & 0xFF;
    output[27] = (timestamp_ns >> 32) & 0xFF;
    output[28] = (timestamp_ns >> 24) & 0xFF;
    output[29] = (timestamp_ns >> 16) & 0xFF;
    output[30] = (timestamp_ns >> 8) & 0xFF;
    output[31] = timestamp_ns & 0xFF;
    
    return true;
}


/*
 * Unpack MEMSHADOW v3 Header
 */
bool memshadow_header_unpack(const uint8_t *input, MemshadowHeader *header) {
    if (!input || !header) {
        return false;
    }
    
    // Unpack big-endian
    header->magic = ((uint64_t)input[0] << 56) | ((uint64_t)input[1] << 48) |
                    ((uint64_t)input[2] << 40) | ((uint64_t)input[3] << 32) |
                    ((uint64_t)input[4] << 24) | ((uint64_t)input[5] << 16) |
                    ((uint64_t)input[6] << 8) | input[7];
    
    header->version = ((uint16_t)input[8] << 8) | input[9];
    header->priority = ((uint16_t)input[10] << 8) | input[11];
    header->msg_type = ((uint16_t)input[12] << 8) | input[13];
    header->flags_batch = ((uint16_t)input[14] << 8) | input[15];
    header->payload_len = ((uint32_t)input[16] << 24) | ((uint32_t)input[17] << 16) |
                          ((uint32_t)input[18] << 8) | input[19];
    header->sequence_num = ((uint32_t)input[20] << 24) | ((uint32_t)input[21] << 16) |
                           ((uint32_t)input[22] << 8) | input[23];
    header->timestamp_ns = ((uint64_t)input[24] << 56) | ((uint64_t)input[25] << 48) |
                           ((uint64_t)input[26] << 40) | ((uint64_t)input[27] << 32) |
                           ((uint64_t)input[28] << 24) | ((uint64_t)input[29] << 16) |
                           ((uint64_t)input[30] << 8) | input[31];
    
    // Validate magic
    if ((header->magic >> 32) != 0x4D534857) {  // "MSHW"
        return false;
    }
    
    // Validate version
    if (header->version != 2 && header->version != 3) {
        return false;
    }
    
    return true;
}


/*
 * Compute HMAC-SHA384
 * CNSA 2.0 compliant authentication
 */
bool memshadow_compute_hmac_sha384(const uint8_t *data, size_t data_len,
                                   const uint8_t *key, size_t key_len,
                                   uint8_t *hmac_out) {
    if (!data || !key || !hmac_out) {
        return false;
    }
    
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    bool success = false;
    
    // Acquire crypto context
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return false;
    }
    
    // Create HMAC key
    struct {
        BLOBHEADER hdr;
        DWORD key_size;
        BYTE key_data[256];
    } key_blob;
    
    key_blob.hdr.bType = PLAINTEXTKEYBLOB;
    key_blob.hdr.bVersion = CUR_BLOB_VERSION;
    key_blob.hdr.reserved = 0;
    key_blob.hdr.aiKeyAlg = CALG_RC2;
    key_blob.key_size = (DWORD)key_len;
    memcpy(key_blob.key_data, key, key_len);
    
    if (!CryptImportKey(hProv, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + key_len,
                        0, CRYPT_IPSEC_HMAC_KEY, &hKey)) {
        goto cleanup;
    }
    
    // Create HMAC-SHA384 hash
    if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash)) {
        goto cleanup;
    }
    
    HMAC_INFO hmac_info;
    ZeroMemory(&hmac_info, sizeof(hmac_info));
    hmac_info.HashAlgid = CALG_SHA_384;
    
    if (!CryptSetHashParam(hHash, HP_HMAC_INFO, (BYTE *)&hmac_info, 0)) {
        goto cleanup;
    }
    
    // Hash data
    if (!CryptHashData(hHash, data, (DWORD)data_len, 0)) {
        goto cleanup;
    }
    
    // Get HMAC value (48 bytes for SHA-384)
    DWORD hmac_len = 48;
    uint8_t full_hmac[48];
    if (!CryptGetHashParam(hHash, HP_HASHVAL, full_hmac, &hmac_len, 0)) {
        goto cleanup;
    }
    
    // Use first 16 bytes for MEMSHADOW protocol
    memcpy(hmac_out, full_hmac, 16);
    success = true;
    
cleanup:
    if (hHash) CryptDestroyHash(hHash);
    if (hKey) CryptDestroyKey(hKey);
    if (hProv) CryptReleaseContext(hProv, 0);
    
    SecureZeroMemory(&key_blob, sizeof(key_blob));
    SecureZeroMemory(full_hmac, sizeof(full_hmac));
    
    return success;
}


/*
 * Verify HMAC
 */
bool memshadow_verify_hmac(const uint8_t *data, size_t data_len,
                           const uint8_t *key, size_t key_len,
                           const uint8_t *expected_hmac) {
    uint8_t computed_hmac[16];
    
    if (!memshadow_compute_hmac_sha384(data, data_len, key, key_len, computed_hmac)) {
        return false;
    }
    
    // Constant-time comparison
    int diff = 0;
    for (int i = 0; i < 16; i++) {
        diff |= computed_hmac[i] ^ expected_hmac[i];
    }
    
    SecureZeroMemory(computed_hmac, sizeof(computed_hmac));
    
    return diff == 0;
}


/*
 * Pack APP_REGISTER Message
 */
bool memshadow_pack_app_register(const uint8_t *app_id, const char *name,
                                 const char *capabilities_json,
                                 const uint8_t *session_token,
                                 uint8_t **payload, size_t *payload_len) {
    if (!app_id || !name || !capabilities_json || !payload || !payload_len) {
        return false;
    }
    
    // Generate nonce
    uint64_t nonce = get_timestamp_ns();
    
    // Compute auth
    uint8_t auth[16] = {0};
    if (session_token) {
        uint8_t auth_data[8 + 8];
        memcpy(auth_data, session_token, 8);
        memcpy(auth_data + 8, &nonce, 8);
        memshadow_compute_hmac_sha384(auth_data, sizeof(auth_data), session_token, 16, auth);
    }
    
    size_t name_len = strlen(name);
    size_t cap_len = strlen(capabilities_json);
    
    if (name_len > 64) {
        return false;
    }
    
    // Calculate payload size
    *payload_len = 16 + 8 + 16 + 2 + 1 + name_len + cap_len;
    *payload = (uint8_t *)malloc(*payload_len);
    if (!*payload) {
        return false;
    }
    
    size_t offset = 0;
    
    // auth (16 bytes)
    memcpy(*payload + offset, auth, 16);
    offset += 16;
    
    // nonce (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (nonce >> (i * 8)) & 0xFF;
    }
    
    // app_id (16 bytes)
    memcpy(*payload + offset, app_id, 16);
    offset += 16;
    
    // capabilities_len (2 bytes, big-endian)
    (*payload)[offset++] = (cap_len >> 8) & 0xFF;
    (*payload)[offset++] = cap_len & 0xFF;
    
    // name_len (1 byte)
    (*payload)[offset++] = (uint8_t)name_len;
    
    // name
    memcpy(*payload + offset, name, name_len);
    offset += name_len;
    
    // capabilities_json
    memcpy(*payload + offset, capabilities_json, cap_len);
    
    return true;
}


/*
 * Unpack APP_REGISTER Message
 */
bool memshadow_unpack_app_register(const uint8_t *payload, size_t payload_len,
                                   uint8_t *app_id, char *name, size_t name_size,
                                   char *capabilities_json, size_t cap_size) {
    if (!payload || !app_id || !name || !capabilities_json) {
        return false;
    }
    
    if (payload_len < 16 + 8 + 16 + 2 + 1) {
        return false;
    }
    
    size_t offset = 0;
    
    // Skip auth (16 bytes)
    offset += 16;
    
    // Skip nonce (8 bytes)
    offset += 8;
    
    // app_id (16 bytes)
    memcpy(app_id, payload + offset, 16);
    offset += 16;
    
    // capabilities_len (2 bytes, big-endian)
    uint16_t cap_len = ((uint16_t)payload[offset] << 8) | payload[offset + 1];
    offset += 2;
    
    // name_len (1 byte)
    uint8_t name_len = payload[offset++];
    
    if (offset + name_len + cap_len > payload_len) {
        return false;
    }
    
    // name
    size_t copy_name_len = (name_len < name_size - 1) ? name_len : (name_size - 1);
    memcpy(name, payload + offset, copy_name_len);
    name[copy_name_len] = '\0';
    offset += name_len;
    
    // capabilities_json
    size_t copy_cap_len = (cap_len < cap_size - 1) ? cap_len : (cap_size - 1);
    memcpy(capabilities_json, payload + offset, copy_cap_len);
    capabilities_json[copy_cap_len] = '\0';
    
    return true;
}


/*
 * Pack APP_COMMAND Message
 */
bool memshadow_pack_app_command(const uint8_t *app_id, uint64_t command_id,
                                uint16_t cmd_type, const uint8_t *args, size_t args_len,
                                uint32_t ttl_ms, const uint8_t *session_token,
                                uint8_t **payload, size_t *payload_len) {
    if (!app_id || !args || !payload || !payload_len) {
        return false;
    }
    
    // Generate nonce
    uint64_t nonce = get_timestamp_ns();
    
    // Compute auth
    uint8_t auth[16] = {0};
    if (session_token) {
        uint8_t auth_data[8 + 8];
        memcpy(auth_data, session_token, 8);
        memcpy(auth_data + 8, &nonce, 8);
        memshadow_compute_hmac_sha384(auth_data, sizeof(auth_data), session_token, 16, auth);
    }
    
    // Calculate payload size
    *payload_len = 16 + 8 + 16 + 8 + 4 + 2 + 2 + args_len;
    *payload = (uint8_t *)malloc(*payload_len);
    if (!*payload) {
        return false;
    }
    
    size_t offset = 0;
    
    // auth (16 bytes)
    memcpy(*payload + offset, auth, 16);
    offset += 16;
    
    // nonce (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (nonce >> (i * 8)) & 0xFF;
    }
    
    // app_id (16 bytes)
    memcpy(*payload + offset, app_id, 16);
    offset += 16;
    
    // command_id (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (command_id >> (i * 8)) & 0xFF;
    }
    
    // ttl_ms (4 bytes, big-endian)
    (*payload)[offset++] = (ttl_ms >> 24) & 0xFF;
    (*payload)[offset++] = (ttl_ms >> 16) & 0xFF;
    (*payload)[offset++] = (ttl_ms >> 8) & 0xFF;
    (*payload)[offset++] = ttl_ms & 0xFF;
    
    // cmd_type (2 bytes, big-endian)
    (*payload)[offset++] = (cmd_type >> 8) & 0xFF;
    (*payload)[offset++] = cmd_type & 0xFF;
    
    // arg_len (2 bytes, big-endian)
    (*payload)[offset++] = (args_len >> 8) & 0xFF;
    (*payload)[offset++] = args_len & 0xFF;
    
    // args
    memcpy(*payload + offset, args, args_len);
    
    return true;
}


/*
 * Unpack APP_COMMAND Message
 */
bool memshadow_unpack_app_command(const uint8_t *payload, size_t payload_len,
                                  uint8_t *app_id, uint64_t *command_id,
                                  uint16_t *cmd_type, uint32_t *ttl_ms,
                                  uint8_t **args, size_t *args_len) {
    if (!payload || !app_id || !command_id || !cmd_type || !ttl_ms || !args || !args_len) {
        return false;
    }
    
    if (payload_len < 16 + 8 + 16 + 8 + 4 + 2 + 2) {
        return false;
    }
    
    size_t offset = 0;
    
    // Skip auth (16 bytes)
    offset += 16;
    
    // Skip nonce (8 bytes)
    offset += 8;
    
    // app_id (16 bytes)
    memcpy(app_id, payload + offset, 16);
    offset += 16;
    
    // command_id (8 bytes, big-endian)
    *command_id = 0;
    for (int i = 0; i < 8; i++) {
        *command_id = (*command_id << 8) | payload[offset++];
    }
    
    // ttl_ms (4 bytes, big-endian)
    *ttl_ms = ((uint32_t)payload[offset] << 24) | ((uint32_t)payload[offset + 1] << 16) |
              ((uint32_t)payload[offset + 2] << 8) | payload[offset + 3];
    offset += 4;
    
    // cmd_type (2 bytes, big-endian)
    *cmd_type = ((uint16_t)payload[offset] << 8) | payload[offset + 1];
    offset += 2;
    
    // arg_len (2 bytes, big-endian)
    *args_len = ((uint16_t)payload[offset] << 8) | payload[offset + 1];
    offset += 2;
    
    if (offset + *args_len > payload_len) {
        return false;
    }
    
    // args
    *args = (uint8_t *)malloc(*args_len);
    if (!*args) {
        return false;
    }
    memcpy(*args, payload + offset, *args_len);
    
    return true;
}


/*
 * Pack APP_COMMAND_ACK Message
 */
bool memshadow_pack_app_command_ack(const uint8_t *app_id, uint64_t command_id,
                                    uint8_t status, const uint8_t *result, size_t result_len,
                                    const uint8_t *session_token,
                                    uint8_t **payload, size_t *payload_len) {
    if (!app_id || !result || !payload || !payload_len) {
        return false;
    }
    
    // Generate nonce
    uint64_t nonce = get_timestamp_ns();
    
    // Compute auth
    uint8_t auth[16] = {0};
    if (session_token) {
        uint8_t auth_data[8 + 8];
        memcpy(auth_data, session_token, 8);
        memcpy(auth_data + 8, &nonce, 8);
        memshadow_compute_hmac_sha384(auth_data, sizeof(auth_data), session_token, 16, auth);
    }
    
    // Calculate payload size
    *payload_len = 16 + 8 + 16 + 8 + 1 + 2 + result_len;
    *payload = (uint8_t *)malloc(*payload_len);
    if (!*payload) {
        return false;
    }
    
    size_t offset = 0;
    
    // auth (16 bytes)
    memcpy(*payload + offset, auth, 16);
    offset += 16;
    
    // nonce (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (nonce >> (i * 8)) & 0xFF;
    }
    
    // app_id (16 bytes)
    memcpy(*payload + offset, app_id, 16);
    offset += 16;
    
    // command_id (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (command_id >> (i * 8)) & 0xFF;
    }
    
    // status (1 byte)
    (*payload)[offset++] = status;
    
    // result_len (2 bytes, big-endian)
    (*payload)[offset++] = (result_len >> 8) & 0xFF;
    (*payload)[offset++] = result_len & 0xFF;
    
    // result
    memcpy(*payload + offset, result, result_len);
    
    return true;
}


/*
 * Pack APP_HEARTBEAT Message
 */
bool memshadow_pack_app_heartbeat(const uint8_t *app_id, uint64_t uptime_ms,
                                  uint8_t load_pct, uint8_t temp_c,
                                  const uint8_t *session_token,
                                  uint8_t **payload, size_t *payload_len) {
    if (!app_id || !payload || !payload_len) {
        return false;
    }
    
    // Generate nonce
    uint64_t nonce = get_timestamp_ns();
    
    // Compute auth
    uint8_t auth[16] = {0};
    if (session_token) {
        uint8_t auth_data[8 + 8];
        memcpy(auth_data, session_token, 8);
        memcpy(auth_data + 8, &nonce, 8);
        memshadow_compute_hmac_sha384(auth_data, sizeof(auth_data), session_token, 16, auth);
    }
    
    // Calculate payload size
    *payload_len = 16 + 8 + 16 + 8 + 1 + 1;
    *payload = (uint8_t *)malloc(*payload_len);
    if (!*payload) {
        return false;
    }
    
    size_t offset = 0;
    
    // auth (16 bytes)
    memcpy(*payload + offset, auth, 16);
    offset += 16;
    
    // nonce (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (nonce >> (i * 8)) & 0xFF;
    }
    
    // app_id (16 bytes)
    memcpy(*payload + offset, app_id, 16);
    offset += 16;
    
    // uptime_ms (8 bytes, big-endian)
    for (int i = 7; i >= 0; i--) {
        (*payload)[offset++] = (uptime_ms >> (i * 8)) & 0xFF;
    }
    
    // load_pct (1 byte)
    (*payload)[offset++] = load_pct;
    
    // temp_c (1 byte)
    (*payload)[offset++] = temp_c;
    
    return true;
}


/*
 * Create Complete MEMSHADOW Message
 * Combines header and payload
 */
bool memshadow_create_message(uint16_t priority, uint8_t flags, uint16_t msg_type,
                              uint8_t batch_count, const uint8_t *payload, size_t payload_len,
                              uint32_t sequence_num,
                              uint8_t **message, size_t *message_len) {
    if (!payload || !message || !message_len) {
        return false;
    }
    
    *message_len = MEMSHADOW_HEADER_SIZE + payload_len;
    *message = (uint8_t *)malloc(*message_len);
    if (!*message) {
        return false;
    }
    
    // Pack header
    if (!memshadow_header_pack(priority, flags, msg_type, batch_count, 
                              (uint32_t)payload_len, sequence_num, *message)) {
        free(*message);
        return false;
    }
    
    // Copy payload
    memcpy(*message + MEMSHADOW_HEADER_SIZE, payload, payload_len);
    
    return true;
}


/*
 * Parse MEMSHADOW Message
 * Separates header and payload
 */
bool memshadow_parse_message(const uint8_t *message, size_t message_len,
                             MemshadowHeader *header,
                             uint8_t **payload, size_t *payload_len) {
    if (!message || !header || !payload || !payload_len) {
        return false;
    }
    
    if (message_len < MEMSHADOW_HEADER_SIZE) {
        return false;
    }
    
    // Unpack header
    if (!memshadow_header_unpack(message, header)) {
        return false;
    }
    
    // Extract payload
    *payload_len = header->payload_len;
    if (message_len < MEMSHADOW_HEADER_SIZE + *payload_len) {
        return false;
    }
    
    *payload = (uint8_t *)malloc(*payload_len);
    if (!*payload) {
        return false;
    }
    
    memcpy(*payload, message + MEMSHADOW_HEADER_SIZE, *payload_len);
    
    return true;
}
