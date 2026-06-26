#include "memshadow.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int hex_to_bytes(const char *hex, uint8_t **out, size_t *out_len) {
    size_t len = strlen(hex);
    size_t i;
    if ((len % 2) != 0) return -1;
    *out_len = len / 2;
    *out = malloc(*out_len);
    if (!*out) return -1;
    for (i = 0; i < *out_len; i++) {
        int hi = hex_value(hex[i * 2]);
        int lo = hex_value(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            free(*out);
            *out = NULL;
            *out_len = 0;
            return -1;
        }
        (*out)[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

static void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
}

int main(int argc, char **argv) {
    uint8_t *data = NULL;
    uint8_t *key = NULL;
    size_t data_len = 0;
    size_t key_len = 0;
    memshadow_header_t header;
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <header|message> <hex> [keyhex]\n", argv[0]);
        return 2;
    }
    if (hex_to_bytes(argv[2], &data, &data_len) != 0) {
        fprintf(stderr, "invalid hex\n");
        return 2;
    }
    if (argc == 4 && hex_to_bytes(argv[3], &key, &key_len) != 0) {
        fprintf(stderr, "invalid key hex\n");
        free(data);
        return 2;
    }

    if (strcmp(argv[1], "header") == 0) {
        uint8_t repacked[MEMSHADOW_HEADER_SIZE];
        if (memshadow_unpack_header(data, data_len, &header) != 0) {
            fprintf(stderr, "header unpack failed\n");
            free(data);
            return 1;
        }
        if (memshadow_pack_header(&header, repacked, sizeof(repacked)) < 0) {
            fprintf(stderr, "header repack failed\n");
            free(data);
            free(key);
            return 1;
        }
        printf("{\"kind\":\"header\",\"magic\":%llu,\"version\":%u,\"priority\":%u,\"msg_type\":%u,\"flags\":%u,\"batch_count\":%u,\"payload_len\":%u,\"sequence_num\":%u,\"repacked_hex\":\"",
               (unsigned long long)header.magic,
               (unsigned int)header.version,
               (unsigned int)header.priority,
               (unsigned int)header.msg_type,
               (unsigned int)(header.flags_batch & 0x00FF),
               (unsigned int)((header.flags_batch >> 8) & 0x00FF),
               (unsigned int)header.payload_len,
               (unsigned int)header.sequence_num);
        print_hex(repacked, sizeof(repacked));
        printf("\"}\n");
    } else if (strcmp(argv[1], "message") == 0) {
        uint8_t payload[65536];
        uint8_t repacked[131072];
        size_t payload_len = 0;
        size_t repacked_len = 0;
        if (memshadow_unpack_message(data, data_len, &header, payload, sizeof(payload), &payload_len, key, key_len) != 0) {
            fprintf(stderr, "message unpack failed\n");
            free(data);
            free(key);
            return 1;
        }
        if (memshadow_pack_message(&header, payload, payload_len, repacked, sizeof(repacked), &repacked_len, key, key_len) != 0) {
            fprintf(stderr, "message repack failed\n");
            free(data);
            free(key);
            return 1;
        }
        printf("{\"kind\":\"message\",\"version\":%u,\"priority\":%u,\"msg_type\":%u,\"flags\":%u,\"batch_count\":%u,\"raw_flags_batch\":%u,\"payload_len\":%u,\"sequence_num\":%u,\"payload_hex\":\"",
               (unsigned int)header.version,
               (unsigned int)header.priority,
               (unsigned int)header.msg_type,
               (unsigned int)(header.flags_batch & 0x00FF),
               (unsigned int)((header.flags_batch >> 8) & 0x00FF),
               (unsigned int)header.flags_batch,
               (unsigned int)payload_len,
               (unsigned int)header.sequence_num);
        for (size_t i = 0; i < payload_len; i++) {
            printf("%02x", payload[i]);
        }
        printf("\",\"repacked_hex\":\"");
        print_hex(repacked, repacked_len);
        printf("\"}\n");
    } else {
        fprintf(stderr, "unknown kind\n");
        free(data);
        free(key);
        return 2;
    }

    free(data);
    free(key);
    return 0;
}
