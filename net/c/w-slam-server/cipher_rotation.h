/*
 * Cipher Rotation Header
 */

#ifndef CIPHER_ROTATION_H
#define CIPHER_ROTATION_H

#include <windows.h>
#include <stdbool.h>

typedef enum {
    CIPHER_KASUMI = 0,
    CIPHER_MISTY1 = 1,
    CIPHER_SEED = 2
} CipherType;

typedef enum {
    ROTATION_TIME_BASED = 0,
    ROTATION_MESSAGE_BASED = 1,
    ROTATION_TRAFFIC_BASED = 2,
    ROTATION_RANDOM = 3
} RotationPolicy;

typedef struct {
    CipherType current_cipher;
    RotationPolicy policy;
    uint32_t rotation_interval;
    uint32_t messages_since_rotation;
    time_t last_rotation_time;
    float network_entropy;
} CipherRotationState;

bool cipher_rotation_init(CipherRotationState *state, RotationPolicy policy, uint32_t interval);
bool cipher_rotation_encrypt(CipherRotationState *state,
                             const uint8_t *plaintext, size_t plaintext_len,
                             const uint8_t *key, const uint8_t *iv,
                             uint8_t **ciphertext, size_t *ciphertext_len,
                             CipherType *cipher_used);
bool cipher_rotation_decrypt(CipherType cipher_type,
                             const uint8_t *ciphertext, size_t ciphertext_len,
                             const uint8_t *key, const uint8_t *iv,
                             uint8_t **plaintext, size_t *plaintext_len);
const char* cipher_rotation_get_cipher_name(CipherType cipher);

#endif // CIPHER_ROTATION_H
