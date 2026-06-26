/*
 * Kasumi Cipher Header
 * 
 * 64-bit block cipher with 128-bit keys (3GPP standard)
 * Legitimate-looking cipher for stealth communications
 */

#ifndef KASUMI_CIPHER_H
#define KASUMI_CIPHER_H

#include <windows.h>
#include <stdbool.h>

#define KASUMI_KEY_SIZE 16      // 128 bits
#define KASUMI_BLOCK_SIZE 8     // 64 bits

/*
 * Kasumi CBC Mode Encryption
 * 
 * Args:
 *   plaintext: Input data
 *   plaintext_len: Input length
 *   key: 128-bit encryption key
 *   iv: 64-bit initialization vector
 *   ciphertext: Output buffer (allocated by function)
 *   ciphertext_len: Output length
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_encrypt_cbc(const uint8_t *plaintext, size_t plaintext_len,
                        const uint8_t *key, const uint8_t *iv,
                        uint8_t **ciphertext, size_t *ciphertext_len);

/*
 * Kasumi CBC Mode Decryption
 * 
 * Args:
 *   ciphertext: Encrypted data
 *   ciphertext_len: Encrypted length
 *   key: 128-bit decryption key
 *   iv: 64-bit initialization vector
 *   plaintext: Output buffer (allocated by function)
 *   plaintext_len: Output length (after padding removal)
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_decrypt_cbc(const uint8_t *ciphertext, size_t ciphertext_len,
                        const uint8_t *key, const uint8_t *iv,
                        uint8_t **plaintext, size_t *plaintext_len);

/*
 * Generate Random IV
 * 
 * Args:
 *   iv: Buffer for IV (must be KASUMI_BLOCK_SIZE bytes)
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_generate_iv(uint8_t *iv);

/*
 * Derive Kasumi Key from Password
 * Uses PBKDF2-HMAC-SHA384 with 100,000 iterations (CNSA 2.0 compliant)
 * 
 * Args:
 *   password: Password string
 *   password_len: Password length
 *   salt: Salt bytes
 *   salt_len: Salt length
 *   iterations: PBKDF2 iterations (recommended: 100,000)
 *   derived_key: Output buffer (must be KASUMI_KEY_SIZE bytes)
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_derive_key(const char *password, size_t password_len,
                       const uint8_t *salt, size_t salt_len,
                       uint32_t iterations, uint8_t *derived_key);

/*
 * High-Level Encryption with Key Derivation
 * 
 * Args:
 *   plaintext: Input data
 *   plaintext_len: Input length
 *   password: Password for key derivation
 *   password_len: Password length
 *   ciphertext: Output buffer (allocated by function)
 *   ciphertext_len: Output length
 *   iv_out: Optional IV output (must be KASUMI_BLOCK_SIZE bytes)
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_encrypt(const uint8_t *plaintext, size_t plaintext_len,
                    const char *password, size_t password_len,
                    uint8_t **ciphertext, size_t *ciphertext_len,
                    uint8_t *iv_out);

/*
 * High-Level Decryption with Key Derivation
 * 
 * Format: [IV:8][Salt:16][Ciphertext:variable]
 * IV and salt are extracted from ciphertext automatically.
 * 
 * Args:
 *   ciphertext: Encrypted data (includes IV and salt)
 *   ciphertext_len: Encrypted length (includes IV and salt)
 *   password: Password for key derivation
 *   password_len: Password length
 *   iv: Optional IV override (if NULL, extracts from ciphertext)
 *   plaintext: Output buffer (allocated by function)
 *   plaintext_len: Output length
 * 
 * Returns:
 *   true on success, false on failure
 */
bool kasumi_decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                    const char *password, size_t password_len,
                    const uint8_t *iv,
                    uint8_t **plaintext, size_t *plaintext_len);

#endif // KASUMI_CIPHER_H
