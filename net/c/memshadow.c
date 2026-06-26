/**
 * MEMSHADOW Protocol v3.0 - C Implementation
 * 
 * Binary protocol implementation for C.
 * All values use big-endian (network byte order).
 */

#define _POSIX_C_SOURCE 200809L
#include "memshadow.h"
#include <string.h>
#include <time.h>
#include <zlib.h>
#include <openssl/hmac.h>

/* HMAC support - using standalone implementation to avoid OpenSSL linking issues */
#define HMAC_AVAILABLE 1

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <endian.h>
#endif

/* Error Messages */
static const char *error_messages[] = {
    [ERR_INVALID_MAGIC] = "Invalid magic number in message header",
    [ERR_INVALID_VERSION] = "Invalid or unsupported protocol version",
    [ERR_INVALID_HEADER] = "Invalid message header format",
    [ERR_INVALID_PAYLOAD] = "Invalid payload format or length mismatch",
    [ERR_INVALID_MESSAGE_TYPE] = "Unknown or unsupported message type",
    [ERR_MESSAGE_TOO_LARGE] = "Message exceeds maximum size",
    [ERR_INVALID_SIGNATURE] = "Message signature verification failed",
    [ERR_AUTHENTICATION_FAILED] = "Peer authentication failed",
    [ERR_KEY_EXCHANGE_FAILED] = "Key exchange failed",
    [ERR_PEER_NOT_FOUND] = "Peer not found in network",
    [ERR_CONNECTION_FAILED] = "Connection establishment failed",
    [ERR_NAT_TRAVERSAL_FAILED] = "NAT traversal failed",
    [ERR_HANDSHAKE_TIMEOUT] = "Handshake timeout (60 seconds)",
    [ERR_VERSION_INCOMPATIBLE] = "Protocol version incompatible",
    [ERR_RATE_LIMIT_EXCEEDED] = "Rate limit exceeded",
    [ERR_FILE_TRANSFER_FAILED] = "File transfer failed",
    [ERR_COMPRESSION_FAILED] = "Compression failed",
    [ERR_HARDWARE_UNAVAILABLE] = "Hardware unavailable (QAT/Kanzi)",
    [ERR_INTERNAL_ERROR] = "Internal error",
};

/* Endianness helpers */
uint16_t memshadow_htobe16(uint16_t host_value) {
#ifdef _WIN32
    return htons(host_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(host_value);
#else
    return host_value;
#endif
}

uint32_t memshadow_htobe32(uint32_t host_value) {
#ifdef _WIN32
    return htonl(host_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(host_value);
#else
    return host_value;
#endif
}

uint64_t memshadow_htobe64(uint64_t host_value) {
#ifdef _WIN32
    return htonll(host_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(host_value);
#else
    return host_value;
#endif
}

uint16_t memshadow_be16toh(uint16_t be_value) {
#ifdef _WIN32
    return ntohs(be_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(be_value);
#else
    return be_value;
#endif
}

uint32_t memshadow_be32toh(uint32_t be_value) {
#ifdef _WIN32
    return ntohl(be_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(be_value);
#else
    return be_value;
#endif
}

uint64_t memshadow_be64toh(uint64_t be_value) {
#ifdef _WIN32
    return ntohll(be_value);
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(be_value);
#else
    return be_value;
#endif
}

/* Pack header to binary (big-endian) */
int memshadow_pack_header(const memshadow_header_t *header, uint8_t *buffer, size_t buffer_size) {
    if (!header || !buffer || buffer_size < MEMSHADOW_HEADER_SIZE) {
        return -1;
    }
    
    uint64_t magic_be = memshadow_htobe64(header->magic);
    uint16_t version_be = memshadow_htobe16(header->version);
    uint16_t priority_be = memshadow_htobe16(header->priority);
    uint16_t msg_type_be = memshadow_htobe16(header->msg_type);
    uint16_t flags_batch_be = memshadow_htobe16(header->flags_batch);
    uint32_t payload_len_be = memshadow_htobe32(header->payload_len);
    uint32_t sequence_num_be = memshadow_htobe32(header->sequence_num);
    uint64_t timestamp_ns_be = memshadow_htobe64(header->timestamp_ns);
    
    memcpy(buffer, &magic_be, 8);
    memcpy(buffer + 8, &version_be, 2);
    memcpy(buffer + 10, &priority_be, 2);
    memcpy(buffer + 12, &msg_type_be, 2);
    memcpy(buffer + 14, &flags_batch_be, 2);
    memcpy(buffer + 16, &payload_len_be, 4);
    memcpy(buffer + 20, &sequence_num_be, 4);
    memcpy(buffer + 24, &timestamp_ns_be, 8);
    
    return MEMSHADOW_HEADER_SIZE;
}

/* Unpack header from binary (big-endian) */
int memshadow_unpack_header(const uint8_t *buffer, size_t buffer_size, memshadow_header_t *header) {
    if (!buffer || !header || buffer_size < MEMSHADOW_HEADER_SIZE) {
        return -1;
    }
    
    uint64_t magic_be, timestamp_ns_be;
    uint16_t version_be, priority_be, msg_type_be, flags_batch_be;
    uint32_t payload_len_be, sequence_num_be;
    
    memcpy(&magic_be, buffer, 8);
    memcpy(&version_be, buffer + 8, 2);
    memcpy(&priority_be, buffer + 10, 2);
    memcpy(&msg_type_be, buffer + 12, 2);
    memcpy(&flags_batch_be, buffer + 14, 2);
    memcpy(&payload_len_be, buffer + 16, 4);
    memcpy(&sequence_num_be, buffer + 20, 4);
    memcpy(&timestamp_ns_be, buffer + 24, 8);
    
    header->magic = memshadow_be64toh(magic_be);
    header->version = memshadow_be16toh(version_be);
    header->priority = memshadow_be16toh(priority_be);
    header->msg_type = memshadow_be16toh(msg_type_be);
    header->flags_batch = memshadow_be16toh(flags_batch_be);
    header->payload_len = memshadow_be32toh(payload_len_be);
    header->timestamp_ns = memshadow_be64toh(timestamp_ns_be);
    header->sequence_num = memshadow_be32toh(sequence_num_be);
    
    return 0;
}

/* Validate header */
bool memshadow_validate_header(const memshadow_header_t *header) {
    if (!header) {
        return false;
    }
    
    // Check magic (first 4 bytes should be "MSHW")
    uint32_t magic_4bytes = (header->magic >> 32) & 0xFFFFFFFF;
    if (magic_4bytes != 0x4D534857) {
        return false;
    }
    
    // Check version major
    uint8_t version_major = (header->version >> 8) & 0xFF;
    if (version_major != MEMSHADOW_VERSION_MAJOR) {
        return false;
    }
    
    return true;
}

/* Create error message */
int memshadow_create_error_message(
    memshadow_error_code_t error_code,
    const char *details,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *message_size
) {
    if (!buffer || !message_size) {
        return -1;
    }
    
    size_t details_len = details ? strlen(details) : 0;
    size_t required_size = 4 + details_len; // error_code (2) + details_len (2) + details
    
    if (buffer_size < required_size) {
        return -1;
    }
    
    uint16_t error_code_be = memshadow_htobe16((uint16_t)error_code);
    uint16_t details_len_be = memshadow_htobe16((uint16_t)details_len);
    
    memcpy(buffer, &error_code_be, 2);
    memcpy(buffer + 2, &details_len_be, 2);
    
    if (details_len > 0) {
        memcpy(buffer + 4, details, details_len);
    }
    
    *message_size = required_size;
    return 0;
}

/* Parse error message */
int memshadow_parse_error_message(
    const uint8_t *buffer,
    size_t buffer_size,
    memshadow_error_code_t *error_code,
    char *details_buffer,
    size_t details_buffer_size
) {
    if (!buffer || buffer_size < 4 || !error_code) {
        return -1;
    }
    
    uint16_t error_code_be, details_len_be;
    memcpy(&error_code_be, buffer, 2);
    memcpy(&details_len_be, buffer + 2, 2);
    
    *error_code = (memshadow_error_code_t)memshadow_be16toh(error_code_be);
    uint16_t details_len = memshadow_be16toh(details_len_be);
    
    if (details_len > 0) {
        if (details_buffer && details_buffer_size > details_len) {
            memcpy(details_buffer, buffer + 4, details_len);
            details_buffer[details_len] = '\0';
        }
    } else if (details_buffer) {
        details_buffer[0] = '\0';
    }
    
    return 0;
}

/* Get error message string */
const char *memshadow_get_error_message(memshadow_error_code_t error_code) {
    if (error_code < sizeof(error_messages) / sizeof(error_messages[0])) {
        return error_messages[error_code];
    }
    return "Unknown error code";
}

/* Check version compatibility */
memshadow_version_compat_t memshadow_check_version_compatibility(
    const memshadow_version_t *local_version,
    const memshadow_version_t *peer_version
) {
    if (!local_version || !peer_version) {
        return VERSION_INCOMPATIBLE_MAJOR;
    }
    
    if (local_version->major == peer_version->major) {
        if (local_version->minor == peer_version->minor) {
            return VERSION_COMPATIBLE;
        } else if (local_version->minor > peer_version->minor) {
            return VERSION_UPGRADE_REQUIRED;
        } else {
            return VERSION_BACKWARD_COMPATIBLE;
        }
    } else {
        return VERSION_INCOMPATIBLE_MAJOR;
    }
}

/* Compression detection */
bool memshadow_detect_kanzi(void) {
    // Placeholder - would check for Kanzi library
    return false;
}

bool memshadow_detect_qatzip(void) {
    // Placeholder - would check for QAT hardware
    return false;
}

memshadow_compression_t memshadow_get_preferred_compression(void) {
    if (memshadow_detect_qatzip()) {
        return COMPRESS_QATZIP;
    } else if (memshadow_detect_kanzi()) {
        return COMPRESS_KANZI;
    } else {
        return COMPRESS_GZIP;
    }
}

/* Compress data */
int memshadow_compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *compressed_size,
    memshadow_compression_t compression_type
) {
    if (!input || !output || !compressed_size) {
        return -1;
    }
    
    if (compression_type == COMPRESS_NONE) {
        if (output_size < input_size) {
            return -1;
        }
        memcpy(output, input, input_size);
        *compressed_size = input_size;
        return 0;
    }
    
    if (compression_type == COMPRESS_GZIP || compression_type == COMPRESS_ZLIB) {
        uLongf dest_len = (uLongf)output_size;
        int ret = compress2(output, &dest_len, input, input_size, Z_DEFAULT_COMPRESSION);
        if (ret == Z_OK) {
            *compressed_size = dest_len;
            return 0;
        }
        return -1;
    }
    
    // Kanzi/QATZip would be implemented here
    return -1;
}

/* Decompress data */
int memshadow_decompress(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size,
    size_t *decompressed_size,
    memshadow_compression_t compression_type
) {
    if (!input || !output || !decompressed_size) {
        return -1;
    }
    
    if (compression_type == COMPRESS_NONE) {
        if (output_size < input_size) {
            return -1;
        }
        memcpy(output, input, input_size);
        *decompressed_size = input_size;
        return 0;
    }
    
    if (compression_type == COMPRESS_GZIP || compression_type == COMPRESS_ZLIB) {
        uLongf dest_len = (uLongf)output_size;
        int ret = uncompress(output, &dest_len, input, input_size);
        if (ret == Z_OK) {
            *decompressed_size = dest_len;
            return 0;
        }
        return -1;
    }
    
    // Kanzi/QATZip would be implemented here
    return -1;
}

/* Get current timestamp (nanoseconds) */
uint64_t memshadow_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Get next sequence number */
uint32_t memshadow_get_next_sequence(uint32_t *sequence_counter) {
    if (!sequence_counter) {
        return 0;
    }
    uint32_t seq = *sequence_counter;
    *sequence_counter = (*sequence_counter + 1) % 0xFFFFFFFF;
    return seq;
}

/* Simple hash-based MAC for message integrity */
/* Compute HMAC-SHA256 for message integrity */
int memshadow_compute_hmac(
    const uint8_t *data,
    size_t data_size,
    const uint8_t *key,
    size_t key_size,
    uint8_t *hmac_output,
    size_t hmac_output_size
) {
    unsigned int out_len = 0;
    if (!data || !key || !hmac_output || hmac_output_size < 32) {
        return -1;
    }
    if (HMAC(EVP_sha256(), key, (int)key_size, data, data_size, hmac_output, &out_len) == NULL) {
        return -1;
    }
    return out_len == 32 ? 0 : -1;
}

/* Verify HMAC-SHA256 for message integrity */
int memshadow_verify_hmac(
    const uint8_t *data,
    size_t data_size,
    const uint8_t *key,
    size_t key_size,
    const uint8_t *received_hmac,
    size_t received_hmac_size
) {
    if (!data || !key || !received_hmac || received_hmac_size < 32) {
        return -1;
    }
    
    uint8_t computed_hmac[32];
    if (memshadow_compute_hmac(data, data_size, key, key_size, computed_hmac, 32) != 0) {
        return -1;
    }
    
    /* Constant-time comparison */
    uint8_t result = 0;
    for (size_t i = 0; i < 32; i++) {
        result |= computed_hmac[i] ^ received_hmac[i];
    }
    
    return (result == 0) ? 0 : -1;
}

/* Pack message with optional HMAC integrity check */
int memshadow_pack_message(
    const memshadow_header_t *header,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *message_size,
    const uint8_t *integrity_key,
    size_t integrity_key_size
) {
    if (!header || !payload || !buffer || !message_size) {
        return -1;
    }
    
    size_t hmac_size = (integrity_key && integrity_key_size > 0) ? 32 : 0;
    size_t total_size = MEMSHADOW_HEADER_SIZE + payload_size + hmac_size;
    
    if (buffer_size < total_size) {
        return -1;
    }
    
    /* Pack header */
    memshadow_header_t header_copy = *header;
    if (integrity_key && integrity_key_size > 0) {
        header_copy.flags_batch |= FLAG_INTEGRITY_CHECK;
        header_copy.payload_len = payload_size + hmac_size;
    } else {
        header_copy.payload_len = payload_size;
    }
    
    int header_result = memshadow_pack_header(&header_copy, buffer, buffer_size);
    if (header_result < 0) {
        return -1;
    }
    
    /* Copy payload */
    memcpy(buffer + MEMSHADOW_HEADER_SIZE, payload, payload_size);
    
    /* Compute and append HMAC if integrity key provided */
    if (integrity_key && integrity_key_size > 0) {
        uint8_t hmac[32];
        if (memshadow_compute_hmac(
            buffer, MEMSHADOW_HEADER_SIZE + payload_size,
            integrity_key, integrity_key_size,
            hmac, 32
        ) != 0) {
            return -1;
        }
        memcpy(buffer + MEMSHADOW_HEADER_SIZE + payload_size, hmac, 32);
    }
    
    *message_size = total_size;
    return 0;
}

/* Unpack message with optional HMAC integrity verification */
int memshadow_unpack_message(
    const uint8_t *buffer,
    size_t buffer_size,
    memshadow_header_t *header,
    uint8_t *payload_buffer,
    size_t payload_buffer_size,
    size_t *payload_size,
    const uint8_t *integrity_key,
    size_t integrity_key_size
) {
    if (!buffer || !header || !payload_buffer || !payload_size) {
        return -1;
    }
    
    if (buffer_size < MEMSHADOW_HEADER_SIZE) {
        return -1;
    }
    
    /* Unpack header */
    if (memshadow_unpack_header(buffer, buffer_size, header) != 0) {
        return -1;
    }
    
    /* Check if integrity check is enabled */
    bool has_integrity = (header->flags_batch & FLAG_INTEGRITY_CHECK) != 0;
    size_t hmac_size = (has_integrity) ? 32 : 0;
    size_t actual_payload_size = header->payload_len - hmac_size;
    
    if (payload_buffer_size < actual_payload_size) {
        return -1;
    }
    
    if (buffer_size < MEMSHADOW_HEADER_SIZE + header->payload_len) {
        return -1;
    }
    
    /* Extract payload */
    memcpy(payload_buffer, buffer + MEMSHADOW_HEADER_SIZE, actual_payload_size);
    *payload_size = actual_payload_size;
    
    /* Verify HMAC if integrity check enabled */
    if (has_integrity && integrity_key && integrity_key_size > 0) {
        const uint8_t *received_hmac = buffer + MEMSHADOW_HEADER_SIZE + actual_payload_size;
        
        /* Compute HMAC over header + payload (without HMAC) */
        if (memshadow_verify_hmac(
            buffer, MEMSHADOW_HEADER_SIZE + actual_payload_size,
            integrity_key, integrity_key_size,
            received_hmac, 32
        ) != 0) {
            return -2;  /* Integrity check failed */
        }
    }

    return 0;
}

/* Additional endianness functions for compatibility */
uint32_t memshadow_pack_uint32(uint32_t value, uint8_t *buffer) {
    if (!buffer) return 0;

    uint32_t be_value = memshadow_htobe32(value);
    memcpy(buffer, &be_value, sizeof(uint32_t));
    return sizeof(uint32_t);
}

uint32_t memshadow_unpack_uint32(const uint8_t *buffer) {
    if (!buffer) return 0;

    uint32_t be_value;
    memcpy(&be_value, buffer, sizeof(uint32_t));
    return memshadow_be32toh(be_value);
}
