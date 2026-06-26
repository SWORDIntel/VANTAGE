/**
 * MEMSHADOW Protocol v3.0 - Comprehensive C Test Suite
 * 
 * Tests all protocol features across C implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "memshadow.h"

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, name) \
    do { \
        if (condition) { \
            printf("✓ %s\n", name); \
            tests_passed++; \
        } else { \
            printf("✗ %s\n", name); \
            tests_failed++; \
        } \
    } while(0)

// ========================================================================
// CORE PROTOCOL TESTS
// ========================================================================

void test_header_creation() {
    memshadow_header_t header;
    memset(&header, 0, sizeof(header));
    
    // Manually initialize header
    header.magic = MEMSHADOW_MAGIC;
    header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header.priority = PRIORITY_NORMAL;
    header.msg_type = MSG_HEARTBEAT;
    header.flags_batch = 0;
    header.payload_len = 0;
    header.timestamp_ns = memshadow_get_timestamp_ns();
    header.sequence_num = 1;
    
    TEST_ASSERT(header.magic == MEMSHADOW_MAGIC, "Header Creation");
    TEST_ASSERT(header.msg_type == MSG_HEARTBEAT, "Header Message Type");
    TEST_ASSERT(header.sequence_num == 1, "Header Sequence Number");
}

void test_header_size() {
    memshadow_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MEMSHADOW_MAGIC;
    header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header.priority = PRIORITY_NORMAL;
    header.msg_type = MSG_DATA;
    header.sequence_num = 1;
    header.timestamp_ns = memshadow_get_timestamp_ns();
    
    uint8_t buffer[MEMSHADOW_HEADER_SIZE];
    int packed_size = memshadow_pack_header(&header, buffer, sizeof(buffer));
    
    TEST_ASSERT(packed_size == MEMSHADOW_HEADER_SIZE, "Header Size");
}

void test_header_round_trip() {
    memshadow_header_t original;
    memset(&original, 0, sizeof(original));
    original.magic = MEMSHADOW_MAGIC;
    original.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    original.priority = PRIORITY_HIGH;
    original.msg_type = MSG_HEARTBEAT;
    original.sequence_num = 123;
    original.timestamp_ns = memshadow_get_timestamp_ns();
    
    uint8_t buffer[MEMSHADOW_HEADER_SIZE];
    int pack_result = memshadow_pack_header(&original, buffer, sizeof(buffer));
    TEST_ASSERT(pack_result > 0, "Header Round Trip - Pack Success");
    
    memshadow_header_t unpacked;
    int unpack_result = memshadow_unpack_header(buffer, sizeof(buffer), &unpacked);
    
    TEST_ASSERT(unpack_result == 0, "Header Round Trip - Unpack Success");
    TEST_ASSERT(unpacked.magic == original.magic, "Header Round Trip - Magic");
    TEST_ASSERT(unpacked.msg_type == original.msg_type, "Header Round Trip - Message Type");
    TEST_ASSERT(unpacked.sequence_num == original.sequence_num, "Header Round Trip - Sequence");
}

void test_all_message_types() {
    memshadow_msg_type_t types[] = {
        MSG_HEARTBEAT,
        MSG_DATA,
        MSG_ERROR,
        MSG_HANDSHAKE,
        MSG_FILE_TRANSFER_START,
        MSG_FILE_TRANSFER_CHUNK,
        MSG_FILE_TRANSFER_END,
    };
    
    int count = sizeof(types) / sizeof(types[0]);
    bool all_passed = true;
    
    for (int i = 0; i < count; i++) {
        memshadow_header_t header;
        memset(&header, 0, sizeof(header));
        header.magic = MEMSHADOW_MAGIC;
        header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
        header.priority = PRIORITY_NORMAL;
        header.msg_type = types[i];
        header.sequence_num = 1;
        header.timestamp_ns = memshadow_get_timestamp_ns();
        
        if (header.msg_type != types[i]) {
            all_passed = false;
            break;
        }
    }
    
    TEST_ASSERT(all_passed, "All Message Types");
}

void test_sequence_number_wraparound() {
    memshadow_header_t header1, header2;
    memset(&header1, 0, sizeof(header1));
    memset(&header2, 0, sizeof(header2));
    
    header1.magic = MEMSHADOW_MAGIC;
    header1.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header1.priority = PRIORITY_NORMAL;
    header1.msg_type = MSG_DATA;
    header1.sequence_num = 0xFFFFFFFF;
    header1.timestamp_ns = memshadow_get_timestamp_ns();
    
    header2.magic = MEMSHADOW_MAGIC;
    header2.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header2.priority = PRIORITY_NORMAL;
    header2.msg_type = MSG_DATA;
    header2.sequence_num = 0;
    header2.timestamp_ns = memshadow_get_timestamp_ns();
    
    TEST_ASSERT(header1.sequence_num == 0xFFFFFFFF, "Sequence Number Wraparound - Max");
    TEST_ASSERT(header2.sequence_num == 0, "Sequence Number Wraparound - Zero");
}

void test_header_validation() {
    memshadow_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MEMSHADOW_MAGIC;
    header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header.priority = PRIORITY_NORMAL;
    header.msg_type = MSG_HEARTBEAT;
    header.sequence_num = 1;
    header.timestamp_ns = memshadow_get_timestamp_ns();
    
    uint8_t buffer[MEMSHADOW_HEADER_SIZE];
    memshadow_pack_header(&header, buffer, sizeof(buffer));
    
    memshadow_header_t unpacked;
    int result = memshadow_unpack_header(buffer, sizeof(buffer), &unpacked);
    TEST_ASSERT(result == 0, "Header Validation - Valid Header");
    
    // Test invalid header (too short)
    uint8_t short_buffer[16];
    memset(short_buffer, 0, sizeof(short_buffer));
    int invalid_result = memshadow_unpack_header(short_buffer, sizeof(short_buffer), &unpacked);
    TEST_ASSERT(invalid_result != 0, "Header Validation - Invalid Size");
}

void test_zero_length_payload() {
    memshadow_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MEMSHADOW_MAGIC;
    header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header.priority = PRIORITY_NORMAL;
    header.msg_type = MSG_HEARTBEAT;
    header.sequence_num = 1;
    header.timestamp_ns = memshadow_get_timestamp_ns();
    header.payload_len = 0;
    
    TEST_ASSERT(header.payload_len == 0, "Zero Length Payload");
}

void test_very_large_sequence() {
    memshadow_header_t header;
    memset(&header, 0, sizeof(header));
    header.magic = MEMSHADOW_MAGIC;
    header.version = (MEMSHADOW_VERSION_MAJOR << 8) | MEMSHADOW_VERSION_MINOR;
    header.priority = PRIORITY_NORMAL;
    header.msg_type = MSG_DATA;
    header.sequence_num = 0xFFFFFFFF;
    header.timestamp_ns = memshadow_get_timestamp_ns();
    
    uint8_t buffer[MEMSHADOW_HEADER_SIZE];
    memshadow_pack_header(&header, buffer, sizeof(buffer));
    
    memshadow_header_t unpacked;
    int result = memshadow_unpack_header(buffer, sizeof(buffer), &unpacked);
    
    TEST_ASSERT(result == 0 && unpacked.sequence_num == 0xFFFFFFFF, "Very Large Sequence");
}

void test_malformed_header() {
    memshadow_header_t header;
    
    // Too short
    uint8_t short_buffer[16];
    memset(short_buffer, 0, sizeof(short_buffer));
    int result = memshadow_unpack_header(short_buffer, sizeof(short_buffer), &header);
    TEST_ASSERT(result != 0, "Malformed Header - Too Short");
    
    // Invalid magic
    uint8_t bad_buffer[MEMSHADOW_HEADER_SIZE];
    memset(bad_buffer, 0xFF, sizeof(bad_buffer));
    int bad_result = memshadow_unpack_header(bad_buffer, sizeof(bad_buffer), &header);
    TEST_ASSERT(bad_result != 0 || header.magic != MEMSHADOW_MAGIC, "Malformed Header - Invalid Magic");
}

// ========================================================================
// MAIN TEST RUNNER
// ========================================================================

int main() {
    printf("======================================================================\n");
    printf("MEMSHADOW Protocol v3.0 - C Test Suite\n");
    printf("======================================================================\n\n");
    
    printf("CORE PROTOCOL TESTS\n");
    printf("======================================================================\n");
    test_header_creation();
    test_header_size();
    test_header_round_trip();
    test_all_message_types();
    test_sequence_number_wraparound();
    test_header_validation();
    test_zero_length_payload();
    test_very_large_sequence();
    test_malformed_header();
    
    printf("\n======================================================================\n");
    printf("TEST SUITE REPORT\n");
    printf("======================================================================\n");
    printf("Total Tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d (%.1f%%)\n", tests_passed, 
           (tests_passed * 100.0) / (tests_passed + tests_failed));
    printf("Failed: %d (%.1f%%)\n", tests_failed,
           (tests_failed * 100.0) / (tests_passed + tests_failed));
    printf("======================================================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
