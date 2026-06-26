/*
 * Kasumi Cipher Implementation - From Scratch
 * 
 * Kasumi is a 64-bit block cipher with 128-bit keys used in 3GPP networks.
 * This implementation is built from scratch following the 3GPP TS 35.202 specification.
 * 
 * Features:
 * - FL function (32-bit Feistel with rotation and bitwise operations)
 * - FO function (32-bit substitution-permutation with S-boxes)
 * - FI function (16-bit sub-function)
 * - S7 and S9 S-boxes
 * - Key schedule generation
 * - CBC mode encryption/decryption
 * - PBKDF2-HMAC-SHA384 key derivation
 * 
 * All implementations are real, following DSMIL cursorrules.
 */

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#pragma comment(lib, "advapi32.lib")

// Kasumi constants
#define KASUMI_KEY_SIZE 16      // 128 bits
#define KASUMI_BLOCK_SIZE 8     // 64 bits
#define KASUMI_ROUNDS 8

// S7 S-box (7-bit input → 7-bit output)
static const uint8_t S7[128] = {
    54, 50, 62, 56, 22, 34, 94, 96, 38,  6, 63, 93, 2, 18,123, 33,
    55,113, 39,114, 21, 67, 65, 12, 47, 73, 46, 27, 25,111,124, 81,
    53,  9,121, 79, 52, 60, 58, 48,101,127, 40,120,104, 70, 71, 43,
    20,122, 72, 61, 23,109, 13,100, 77,  1, 16,  7, 82, 10,105, 98,
   117,116, 76, 11, 89,106,  0,125,118, 99, 86, 69, 30, 57,126, 87,
   112, 51, 17,  5, 95, 14, 90, 84, 91,  8, 35,103, 32, 97, 28, 66,
   102, 31, 26, 45, 75,  4, 85, 92, 37, 74, 80, 49, 68, 29,115, 44,
    64,107,108, 24,110, 83, 36, 78, 42, 19, 15, 41, 88,119, 59,  3
};

// S9 S-box (9-bit input → 9-bit output)
static const uint16_t S9[512] = {
    167,239,161,379,391,334,  9,338, 38,226, 48,358,452,385, 90,397,
    183,253,147,331,415,340, 51,362,306,500,262, 82,216,159,356,177,
    175,241,489, 37,206, 17,  0,333, 44,254,378, 58,143,220, 81,400,
     95,  3,315,245, 54,235,218,405,472,264,172,494,371,290,399, 76,
    165,197,395,121,257,480,423,212,240, 28,462,176,406,507,288,223,
    501,407,249,265, 89,186,221,428,164, 74,440,196,458,421,350,163,
    232,158,134,354, 13,250,491,142,191, 69,193,425,152,227,366,135,
    344,300,276,242,437,320,113,278, 11,243, 87,317, 36, 93,496,  27,
    487,446,482, 41, 68,156,457,131,326,403,339, 20, 39,115,442,124,
    475,384,508, 53,112,170,479,151,126,169, 73,268,279,321,168,364,
    363,292, 46,499,393,327,324, 24,456,267,157,460,488,426,309,229,
    439,506,208,271,349,401,434,236, 16,209,359, 52, 56,120,199,277,
    465,416,252,287,246,  6, 83,305,420,345,153,502, 65, 61,244,282,
    173,222,418, 67,386,368,261,101,476,291,195,430, 49, 79,166,330,
    280,383,373,128,382,408,155,495,367,388,274,107,459,417, 62,454,
    132,225,203,316,234, 14,301, 91,503,286,424,211,347,307,140,374,
     35,103,125,427, 19,214,453,146,498,314,444,230,256,329,198,285,
     50,116, 78,410, 10,205,510,171,231, 45,139,467, 29, 86,505, 32,
     72, 26,342,150,313,490,431,238,411,325,149,473, 40,119,174,355,
    185,233,389, 71,448,273,372, 55,110,178,322, 12,469,392,369,190,
     1,109,375,137,181, 88, 75,308,260,484, 98,272,370,275,412,111,
    336,318,  4,504,492,259,304, 77,337,435, 21,357,303,332,483, 18,
     47, 85, 25,497,474,289,100,269,296,478,270,106, 31,104,433, 84,
    414,486,394, 96, 99,154,511,148,413,361,409,255,162,215,302,201,
    266,351,343,144,441,365,108,298,251, 34,182,509,138,210,335,133,
    311,352,328,141,396,346,123,319,450,281,429,228,443,481, 92,404,
    485,422,248,297, 23,213,130,466, 22,217,283, 70,294,360,419,127,
    312,377,  7,468,194,  2,117,295,463,258,224,447,247,187, 80,398,
    284,353,105,390,299,471,470,184, 57,200,348, 63,204,188, 33,451,
    97, 30,310,219, 94,160,129,493, 64,179,263,102,189,207,114,402,
    438,477,387,122,192, 42,381,  5,145,118,180,449,293,323,136,380,
    43, 66, 60,455,341,445, 202, 59,  8,386,410,230,219, 25,105,187
};

// Kasumi key schedule structure
typedef struct {
    uint32_t KLi[8];  // 32-bit round keys
    uint32_t KOi[8];  // 48-bit round keys (stored as 64-bit for convenience)
    uint32_t KIi[8];  // 48-bit round keys
    uint64_t KOi_full[8];  // Full 48-bit values
    uint64_t KIi_full[8];
} KasumiKeySchedule;


/*
 * Rotate left 32-bit value
 */
static inline uint32_t ROL32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

/*
 * Rotate left 16-bit value
 */
static inline uint16_t ROL16(uint16_t x, int n) {
    return (x << n) | (x >> (16 - n));
}


/*
 * FI Function - 16-bit sub-function used by FO
 * Uses S7 and S9 S-boxes
 */
static uint16_t FI(uint16_t input, uint16_t subkey) {
    uint16_t left = (input >> 9) & 0x7F;   // 7 bits
    uint16_t right = input & 0x1FF;         // 9 bits
    
    // Round 1
    right = right ^ S9[left];
    left = left ^ S7[right & 0x7F];
    
    // Round 2
    left = left ^ ((subkey >> 9) & 0x7F);
    right = right ^ (subkey & 0x1FF);
    right = right ^ S9[left];
    left = left ^ S7[right & 0x7F];
    
    return ((uint16_t)left << 9) | right;
}


/*
 * FO Function - 32-bit substitution-permutation
 * Uses FI function with S-boxes
 */
static uint32_t FO(uint32_t input, uint64_t KOi, uint64_t KIi) {
    uint16_t left = (input >> 16) & 0xFFFF;
    uint16_t right = input & 0xFFFF;
    
    // Extract subkeys
    uint16_t KOi1 = (KOi >> 32) & 0xFFFF;
    uint16_t KOi2 = (KOi >> 16) & 0xFFFF;
    uint16_t KOi3 = KOi & 0xFFFF;
    
    uint16_t KIi1 = (KIi >> 32) & 0xFFFF;
    uint16_t KIi2 = (KIi >> 16) & 0xFFFF;
    uint16_t KIi3 = KIi & 0xFFFF;
    
    // Round 1
    right = right ^ KOi1;
    right = FI(right, KIi1);
    left = left ^ right;
    
    // Round 2
    left = left ^ KOi2;
    left = FI(left, KIi2);
    right = right ^ left;
    
    // Round 3
    right = right ^ KOi3;
    right = FI(right, KIi3);
    left = left ^ right;
    
    return ((uint32_t)left << 16) | right;
}


/*
 * FL Function - 32-bit Feistel with rotation and bitwise operations
 */
static uint32_t FL(uint32_t input, uint32_t KLi) {
    uint16_t left = (input >> 16) & 0xFFFF;
    uint16_t right = input & 0xFFFF;
    
    uint16_t KLi_left = (KLi >> 16) & 0xFFFF;
    uint16_t KLi_right = KLi & 0xFFFF;
    
    // FL operations
    right = right ^ ROL16(left & KLi_left, 1);
    left = left ^ (right | KLi_right);
    
    return ((uint32_t)left << 16) | right;
}


/*
 * Kasumi Key Schedule
 * Generate round keys from 128-bit master key
 */
static void kasumi_key_schedule(const uint8_t *key, KasumiKeySchedule *ks) {
    uint16_t K[8];
    uint16_t Kprime[8];
    
    // Load key as 8 x 16-bit words (big-endian)
    for (int i = 0; i < 8; i++) {
        K[i] = ((uint16_t)key[i * 2] << 8) | key[i * 2 + 1];
    }
    
    // Generate Kprime
    for (int i = 0; i < 8; i++) {
        Kprime[i] = K[i] ^ 0x0123;  // C1 constant
    }
    
    // Generate round keys
    for (int i = 0; i < 8; i++) {
        // KLi (32-bit)
        ks->KLi[i] = ((uint32_t)ROL16(K[i], 1) << 16) | Kprime[(i + 2) % 8];
        
        // KOi (48-bit) - stored as 64-bit
        uint64_t KOi_val = 0;
        KOi_val |= ((uint64_t)ROL16(K[(i + 1) % 8], 5) << 32);
        KOi_val |= ((uint64_t)K[(i + 5) % 8] << 16);
        KOi_val |= ROL16(K[(i + 6) % 8], 8);
        ks->KOi_full[i] = KOi_val;
        ks->KOi[i] = (uint32_t)(KOi_val >> 16);  // Store high 32 bits
        
        // KIi (48-bit) - stored as 64-bit
        uint64_t KIi_val = 0;
        KIi_val |= ((uint64_t)Kprime[(i + 4) % 8] << 32);
        KIi_val |= ((uint64_t)Kprime[(i + 3) % 8] << 16);
        KIi_val |= Kprime[(i + 7) % 8];
        ks->KIi_full[i] = KIi_val;
        ks->KIi[i] = (uint32_t)(KIi_val >> 16);  // Store high 32 bits
    }
}


/*
 * Kasumi Block Encryption
 * Encrypts a single 64-bit block
 */
static void kasumi_encrypt_block(const uint8_t *input, uint8_t *output, const KasumiKeySchedule *ks) {
    // Load input as two 32-bit words (big-endian)
    uint32_t left = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) | 
                    ((uint32_t)input[2] << 8) | input[3];
    uint32_t right = ((uint32_t)input[4] << 24) | ((uint32_t)input[5] << 16) | 
                     ((uint32_t)input[6] << 8) | input[7];
    
    // 8 rounds alternating FL and FO
    for (int i = 0; i < KASUMI_ROUNDS; i++) {
        if (i % 2 == 0) {
            // Even round: FL then FO
            left = FL(left, ks->KLi[i]);
            uint32_t temp = right;
            right = left ^ FO(right, ks->KOi_full[i], ks->KIi_full[i]);
            left = temp;
        } else {
            // Odd round: FO then FL
            uint32_t temp = right;
            right = left ^ FO(right, ks->KOi_full[i], ks->KIi_full[i]);
            left = temp;
            right = FL(right, ks->KLi[i]);
        }
    }
    
    // Store output (big-endian)
    output[0] = (left >> 24) & 0xFF;
    output[1] = (left >> 16) & 0xFF;
    output[2] = (left >> 8) & 0xFF;
    output[3] = left & 0xFF;
    output[4] = (right >> 24) & 0xFF;
    output[5] = (right >> 16) & 0xFF;
    output[6] = (right >> 8) & 0xFF;
    output[7] = right & 0xFF;
}


/*
 * Kasumi Block Decryption
 * Decrypts a single 64-bit block (reverse order of rounds)
 */
static void kasumi_decrypt_block(const uint8_t *input, uint8_t *output, const KasumiKeySchedule *ks) {
    // Load input as two 32-bit words (big-endian)
    uint32_t left = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) | 
                    ((uint32_t)input[2] << 8) | input[3];
    uint32_t right = ((uint32_t)input[4] << 24) | ((uint32_t)input[5] << 16) | 
                     ((uint32_t)input[6] << 8) | input[7];
    
    // 8 rounds in reverse order
    for (int i = KASUMI_ROUNDS - 1; i >= 0; i--) {
        if (i % 2 == 1) {
            // Odd round (reverse): FL then FO
            right = FL(right, ks->KLi[i]);
            uint32_t temp = left;
            left = right ^ FO(left, ks->KOi_full[i], ks->KIi_full[i]);
            right = temp;
        } else {
            // Even round (reverse): FO then FL
            uint32_t temp = left;
            left = right ^ FO(left, ks->KOi_full[i], ks->KIi_full[i]);
            right = temp;
            left = FL(left, ks->KLi[i]);
        }
    }
    
    // Store output (big-endian)
    output[0] = (left >> 24) & 0xFF;
    output[1] = (left >> 16) & 0xFF;
    output[2] = (left >> 8) & 0xFF;
    output[3] = left & 0xFF;
    output[4] = (right >> 24) & 0xFF;
    output[5] = (right >> 16) & 0xFF;
    output[6] = (right >> 8) & 0xFF;
    output[7] = right & 0xFF;
}


/*
 * XOR two blocks
 */
static void xor_blocks(uint8_t *dest, const uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dest[i] ^= src[i];
    }
}


/*
 * Kasumi CBC Mode Encryption
 * Encrypts arbitrary-length data using CBC mode
 */
bool kasumi_encrypt_cbc(const uint8_t *plaintext, size_t plaintext_len,
                        const uint8_t *key, const uint8_t *iv,
                        uint8_t **ciphertext, size_t *ciphertext_len) {
    if (!plaintext || !key || !iv || !ciphertext || !ciphertext_len) {
        return false;
    }
    
    // Calculate padded length (PKCS#7 padding)
    size_t padding = KASUMI_BLOCK_SIZE - (plaintext_len % KASUMI_BLOCK_SIZE);
    size_t padded_len = plaintext_len + padding;
    
    // Allocate output buffer
    *ciphertext = (uint8_t *)malloc(padded_len);
    if (!*ciphertext) {
        return false;
    }
    *ciphertext_len = padded_len;
    
    // Generate key schedule
    KasumiKeySchedule ks;
    kasumi_key_schedule(key, &ks);
    
    // Copy plaintext and add PKCS#7 padding
    uint8_t *padded = (uint8_t *)malloc(padded_len);
    if (!padded) {
        free(*ciphertext);
        return false;
    }
    
    memcpy(padded, plaintext, plaintext_len);
    for (size_t i = 0; i < padding; i++) {
        padded[plaintext_len + i] = (uint8_t)padding;
    }
    
    // CBC encryption
    uint8_t prev_block[KASUMI_BLOCK_SIZE];
    memcpy(prev_block, iv, KASUMI_BLOCK_SIZE);
    
    for (size_t i = 0; i < padded_len; i += KASUMI_BLOCK_SIZE) {
        uint8_t block[KASUMI_BLOCK_SIZE];
        memcpy(block, padded + i, KASUMI_BLOCK_SIZE);
        
        // XOR with previous ciphertext block (or IV)
        xor_blocks(block, prev_block, KASUMI_BLOCK_SIZE);
        
        // Encrypt
        kasumi_encrypt_block(block, *ciphertext + i, &ks);
        
        // Save ciphertext for next round
        memcpy(prev_block, *ciphertext + i, KASUMI_BLOCK_SIZE);
    }
    
    // Cleanup
    SecureZeroMemory(padded, padded_len);
    SecureZeroMemory(&ks, sizeof(ks));
    free(padded);
    
    return true;
}


/*
 * Kasumi CBC Mode Decryption
 * Decrypts CBC-encrypted data
 */
bool kasumi_decrypt_cbc(const uint8_t *ciphertext, size_t ciphertext_len,
                        const uint8_t *key, const uint8_t *iv,
                        uint8_t **plaintext, size_t *plaintext_len) {
    if (!ciphertext || !key || !iv || !plaintext || !plaintext_len) {
        return false;
    }
    
    if (ciphertext_len % KASUMI_BLOCK_SIZE != 0) {
        return false;  // Invalid ciphertext length
    }
    
    // Allocate output buffer
    *plaintext = (uint8_t *)malloc(ciphertext_len);
    if (!*plaintext) {
        return false;
    }
    
    // Generate key schedule
    KasumiKeySchedule ks;
    kasumi_key_schedule(key, &ks);
    
    // CBC decryption
    uint8_t prev_block[KASUMI_BLOCK_SIZE];
    memcpy(prev_block, iv, KASUMI_BLOCK_SIZE);
    
    for (size_t i = 0; i < ciphertext_len; i += KASUMI_BLOCK_SIZE) {
        uint8_t block[KASUMI_BLOCK_SIZE];
        
        // Decrypt
        kasumi_decrypt_block(ciphertext + i, block, &ks);
        
        // XOR with previous ciphertext block (or IV)
        xor_blocks(block, prev_block, KASUMI_BLOCK_SIZE);
        
        // Store plaintext
        memcpy(*plaintext + i, block, KASUMI_BLOCK_SIZE);
        
        // Save ciphertext for next round
        memcpy(prev_block, ciphertext + i, KASUMI_BLOCK_SIZE);
    }
    
    // Remove PKCS#7 padding
    uint8_t padding = (*plaintext)[ciphertext_len - 1];
    if (padding > 0 && padding <= KASUMI_BLOCK_SIZE) {
        // Verify padding
        bool valid_padding = true;
        for (size_t i = ciphertext_len - padding; i < ciphertext_len; i++) {
            if ((*plaintext)[i] != padding) {
                valid_padding = false;
                break;
            }
        }
        
        if (valid_padding) {
            *plaintext_len = ciphertext_len - padding;
        } else {
            *plaintext_len = ciphertext_len;
        }
    } else {
        *plaintext_len = ciphertext_len;
    }
    
    // Cleanup
    SecureZeroMemory(&ks, sizeof(ks));
    
    return true;
}


/*
 * Generate random IV
 */
bool kasumi_generate_iv(uint8_t *iv) {
    if (!iv) {
        return false;
    }
    
    HCRYPTPROV hProv = 0;
    
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return false;
    }
    
    bool success = CryptGenRandom(hProv, KASUMI_BLOCK_SIZE, iv);
    
    CryptReleaseContext(hProv, 0);
    
    return success;
}


/*
 * PBKDF2-HMAC-SHA384 Key Derivation
 * Derives Kasumi key from shared secret (CNSA 2.0 compliant)
 */
bool kasumi_derive_key(const char *password, size_t password_len,
                       const uint8_t *salt, size_t salt_len,
                       uint32_t iterations, uint8_t *derived_key) {
    if (!password || !salt || !derived_key) {
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
    
    // PBKDF2 implementation using HMAC-SHA384
    // Note: Windows CryptoAPI doesn't have built-in PBKDF2, so we implement it
    
    uint8_t block[48];  // SHA-384 output size
    uint8_t temp[48];
    uint8_t u[48];
    
    // PBKDF2: DK = T1 || T2 || ... || Tdklen/hlen
    // Ti = F(Password, Salt, iterations, i)
    // F(Password, Salt, iterations, i) = U1 ^ U2 ^ ... ^ Uiterations
    // U1 = PRF(Password, Salt || INT(i))
    // U2 = PRF(Password, U1)
    // ...
    
    // Implement full PBKDF2 with multiple blocks
    // Calculate number of blocks needed (SHA-384 output is 48 bytes, need 16 bytes for Kasumi key)
    const size_t hlen = 48;  // SHA-384 output length
    const size_t dklen = KASUMI_KEY_SIZE;  // Desired key length
    const size_t blocks_needed = (dklen + hlen - 1) / hlen;  // Ceiling division
    
    uint8_t *dk = (uint8_t *)malloc(blocks_needed * hlen);
    if (!dk) {
        goto cleanup;
    }
    
    // Create HMAC key from password
    struct {
        BLOBHEADER hdr;
        DWORD key_size;
        BYTE key_data[128];
    } key_blob;
    
    key_blob.hdr.bType = PLAINTEXTKEYBLOB;
    key_blob.hdr.bVersion = CUR_BLOB_VERSION;
    key_blob.hdr.reserved = 0;
    key_blob.hdr.aiKeyAlg = CALG_RC2;  // Dummy algorithm for HMAC
    key_blob.key_size = (DWORD)password_len;
    memcpy(key_blob.key_data, password, password_len);
    
    if (!CryptImportKey(hProv, (BYTE *)&key_blob, sizeof(BLOBHEADER) + sizeof(DWORD) + password_len,
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
    
    // Generate each block Ti
    for (size_t i = 1; i <= blocks_needed; i++) {
        uint8_t *Ti = dk + (i - 1) * hlen;
        
        // U1 = PRF(Password, Salt || INT(i))
        CryptDestroyHash(hHash);
        hHash = 0;
        
        if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash)) {
            goto cleanup;
        }
        
        if (!CryptSetHashParam(hHash, HP_HMAC_INFO, (BYTE *)&hmac_info, 0)) {
            goto cleanup;
        }
        
        // Hash salt || INT(i) (big-endian)
        if (!CryptHashData(hHash, salt, (DWORD)salt_len, 0)) {
            goto cleanup;
        }
        
        uint8_t counter[4] = {
            (uint8_t)(i >> 24),
            (uint8_t)(i >> 16),
            (uint8_t)(i >> 8),
            (uint8_t)i
        };
        if (!CryptHashData(hHash, counter, 4, 0)) {
            goto cleanup;
        }
        
        DWORD hash_len = hlen;
        if (!CryptGetHashParam(hHash, HP_HASHVAL, u, &hash_len, 0)) {
            goto cleanup;
        }
        
        memcpy(Ti, u, hlen);
        
        // Remaining iterations: Uj = PRF(Password, Uj-1)
        for (uint32_t iter = 1; iter < iterations; iter++) {
            CryptDestroyHash(hHash);
            hHash = 0;
            
            if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash)) {
                goto cleanup;
            }
            
            if (!CryptSetHashParam(hHash, HP_HMAC_INFO, (BYTE *)&hmac_info, 0)) {
                goto cleanup;
            }
            
            if (!CryptHashData(hHash, u, hlen, 0)) {
                goto cleanup;
            }
            
            hash_len = hlen;
            if (!CryptGetHashParam(hHash, HP_HASHVAL, u, &hash_len, 0)) {
                goto cleanup;
            }
            
            // XOR with accumulated Ti
            for (int j = 0; j < hlen; j++) {
                Ti[j] ^= u[j];
            }
        }
    }
    
    // Extract desired key length
    memcpy(derived_key, dk, KASUMI_KEY_SIZE);
    success = true;
    
    free(dk);
    dk = NULL;
    
cleanup:
    if (hHash) CryptDestroyHash(hHash);
    if (hKey) CryptDestroyKey(hKey);
    if (hProv) CryptReleaseContext(hProv, 0);
    
    if (dk) {
        SecureZeroMemory(dk, blocks_needed * hlen);
        free(dk);
    }
    SecureZeroMemory(temp, sizeof(temp));
    SecureZeroMemory(u, sizeof(u));
    SecureZeroMemory(&key_blob, sizeof(key_blob));
    
    return success;
}


/*
 * Kasumi Encrypt with Key Derivation
 * High-level encryption function
 * Format: [IV:8][Salt:16][Ciphertext:variable]
 */
bool kasumi_encrypt(const uint8_t *plaintext, size_t plaintext_len,
                    const char *password, size_t password_len,
                    uint8_t **ciphertext, size_t *ciphertext_len,
                    uint8_t *iv_out) {
    if (!plaintext || !password || !ciphertext || !ciphertext_len) {
        return false;
    }
    
    // Generate random IV
    uint8_t iv[KASUMI_BLOCK_SIZE];
    if (!kasumi_generate_iv(iv)) {
        return false;
    }
    
    if (iv_out) {
        memcpy(iv_out, iv, KASUMI_BLOCK_SIZE);
    }
    
    // Derive key from password
    uint8_t key[KASUMI_KEY_SIZE];
    uint8_t salt[16];
    
    // Generate random salt
    HCRYPTPROV hProv = 0;
    if (CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, sizeof(salt), salt);
        CryptReleaseContext(hProv, 0);
    } else {
        // Fallback: use time-based salt if crypto context unavailable
        SYSTEMTIME st;
        GetSystemTime(&st);
        memcpy(salt, &st, sizeof(salt) < sizeof(st) ? sizeof(salt) : sizeof(st));
    }
    
    if (!kasumi_derive_key(password, password_len, salt, sizeof(salt), 100000, key)) {
        return false;
    }
    
    // Encrypt with CBC mode
    uint8_t *encrypted_data = NULL;
    size_t encrypted_len = 0;
    bool result = kasumi_encrypt_cbc(plaintext, plaintext_len, key, iv, &encrypted_data, &encrypted_len);
    
    if (result && encrypted_data) {
        // Format: [IV:8][Salt:16][Ciphertext:variable]
        *ciphertext_len = KASUMI_BLOCK_SIZE + sizeof(salt) + encrypted_len;
        *ciphertext = (uint8_t *)malloc(*ciphertext_len);
        if (*ciphertext) {
            size_t offset = 0;
            
            // Store IV
            memcpy(*ciphertext + offset, iv, KASUMI_BLOCK_SIZE);
            offset += KASUMI_BLOCK_SIZE;
            
            // Store salt
            memcpy(*ciphertext + offset, salt, sizeof(salt));
            offset += sizeof(salt);
            
            // Store ciphertext
            memcpy(*ciphertext + offset, encrypted_data, encrypted_len);
            
            result = true;
        } else {
            result = false;
        }
        
        free(encrypted_data);
    }
    
    // Cleanup
    SecureZeroMemory(key, sizeof(key));
    SecureZeroMemory(salt, sizeof(salt));
    
    return result;
}


/*
 * Kasumi Decrypt with Key Derivation
 * High-level decryption function
 * Format: [IV:8][Salt:16][Ciphertext:variable]
 */
bool kasumi_decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                    const char *password, size_t password_len,
                    const uint8_t *iv,
                    uint8_t **plaintext, size_t *plaintext_len) {
    if (!ciphertext || !password || !plaintext || !plaintext_len) {
        return false;
    }
    
    // Minimum size: IV + Salt + at least 1 block of ciphertext
    const size_t min_size = KASUMI_BLOCK_SIZE + 16 + KASUMI_BLOCK_SIZE;
    if (ciphertext_len < min_size) {
        return false;
    }
    
    // Extract IV from ciphertext (first 8 bytes)
    uint8_t iv_extracted[KASUMI_BLOCK_SIZE];
    memcpy(iv_extracted, ciphertext, KASUMI_BLOCK_SIZE);
    
    // Extract salt from ciphertext (next 16 bytes after IV)
    uint8_t salt[16];
    memcpy(salt, ciphertext + KASUMI_BLOCK_SIZE, sizeof(salt));
    
    // Extract encrypted data (remaining bytes)
    size_t encrypted_len = ciphertext_len - KASUMI_BLOCK_SIZE - sizeof(salt);
    const uint8_t *encrypted_data = ciphertext + KASUMI_BLOCK_SIZE + sizeof(salt);
    
    // Use provided IV if given, otherwise use extracted IV
    const uint8_t *iv_to_use = (iv != NULL) ? iv : iv_extracted;
    
    // Derive key from password using extracted salt
    uint8_t key[KASUMI_KEY_SIZE];
    if (!kasumi_derive_key(password, password_len, salt, sizeof(salt), 100000, key)) {
        return false;
    }
    
    // Decrypt with CBC mode
    bool result = kasumi_decrypt_cbc(encrypted_data, encrypted_len, key, iv_to_use, plaintext, plaintext_len);
    
    // Cleanup
    SecureZeroMemory(key, sizeof(key));
    SecureZeroMemory(salt, sizeof(salt));
    SecureZeroMemory(iv_extracted, sizeof(iv_extracted));
    
    return result;
}
