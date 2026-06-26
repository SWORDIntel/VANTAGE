/**
 * MEMSHADOW Protocol - Post-Quantum Cryptography (PQC) Header
 *
 * Implements BFV, CKKS, and TFHE homomorphic encryption schemes.
 * Provides hybrid classical + PQC key management.
 * Includes noise budget tracking for homomorphic operations.
 */

#ifndef MEMSHADOW_PQC_H
#define MEMSHADOW_PQC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PQC Algorithm Types */
typedef enum {
    PQC_ALGORITHM_CLASSICAL_ONLY = 0,
    PQC_ALGORITHM_HYBRID_X25519_KYBER = 1,
    PQC_ALGORITHM_HYBRID_ECDSA_DILITHIUM = 2,
    PQC_ALGORITHM_PQC_ONLY = 3,
    PQC_ALGORITHM_BFV_HOMOMORPHIC = 4,
    PQC_ALGORITHM_CKKS_HOMOMORPHIC = 5,
    PQC_ALGORITHM_TFHE_HOMOMORPHIC = 6,
    PQC_ALGORITHM_HYBRID_BFV_CLASSICAL = 7,
    PQC_ALGORITHM_HYBRID_CKKS_CLASSICAL = 8,
} memshadow_pqc_algorithm_t;

/* Hybrid Key Pair Structure */
typedef struct {
    uint8_t *classical_private_key;
    size_t classical_private_key_size;
    uint8_t *classical_public_key;
    size_t classical_public_key_size;
    uint8_t *pqc_private_key;
    size_t pqc_private_key_size;
    uint8_t *pqc_public_key;
    size_t pqc_public_key_size;
    uint8_t *he_public_key;
    size_t he_public_key_size;
    uint8_t *he_private_key;
    size_t he_private_key_size;
    uint8_t *he_evaluation_key;
    size_t he_evaluation_key_size;
    uint8_t *he_relinearization_key;
    size_t he_relinearization_key_size;
    uint8_t *he_galois_key;
    size_t he_galois_key_size;
    memshadow_pqc_algorithm_t algorithm;
} memshadow_hybrid_keypair_t;

/* Homomorphic Ciphertext Structure */
typedef struct {
    uint8_t *ciphertext;
    size_t ciphertext_size;
    memshadow_pqc_algorithm_t algorithm;
    double scale;  /* For CKKS */
    uint32_t level;
    uint32_t noise_budget;
} memshadow_homomorphic_ciphertext_t;

/* PQC Manager Structure */
typedef struct {
    memshadow_pqc_algorithm_t algorithm;
    void *internal_state;  /* Opaque pointer for implementation details */
} memshadow_pqc_manager_t;

/* Error Codes */
typedef enum {
    PQC_SUCCESS = 0,
    PQC_ERROR_INVALID_ALGORITHM = -1,
    PQC_ERROR_INVALID_KEY = -2,
    PQC_ERROR_ENCRYPTION_FAILED = -3,
    PQC_ERROR_DECRYPTION_FAILED = -4,
    PQC_ERROR_OPERATION_FAILED = -5,
    PQC_ERROR_OUT_OF_MEMORY = -6,
    PQC_ERROR_INVALID_PARAMETERS = -7,
    PQC_ERROR_NOISE_BUDGET_EXHAUSTED = -8,
} memshadow_pqc_error_t;

/* Function Declarations */

/* Initialize PQC manager */
memshadow_pqc_error_t memshadow_pqc_init(
    memshadow_pqc_manager_t **manager,
    memshadow_pqc_algorithm_t algorithm
);

/* Cleanup PQC manager */
void memshadow_pqc_cleanup(memshadow_pqc_manager_t *manager);

/* Initialize hybrid keypair */
memshadow_pqc_error_t memshadow_hybrid_keypair_init(
    memshadow_hybrid_keypair_t *keypair,
    memshadow_pqc_algorithm_t algorithm
);

/* Cleanup hybrid keypair */
void memshadow_hybrid_keypair_cleanup(memshadow_hybrid_keypair_t *keypair);

/* Generate hybrid keypair */
memshadow_pqc_error_t memshadow_pqc_generate_keypair(
    memshadow_pqc_manager_t *manager,
    memshadow_hybrid_keypair_t *keypair
);

/* Perform key exchange */
memshadow_pqc_error_t memshadow_pqc_key_exchange(
    memshadow_pqc_manager_t *manager,
    const uint8_t *my_private_key,
    size_t my_private_key_size,
    const uint8_t *peer_public_key,
    size_t peer_public_key_size,
    uint8_t *shared_secret,
    size_t *shared_secret_size
);

/* Initialize homomorphic ciphertext */
memshadow_pqc_error_t memshadow_homomorphic_ciphertext_init(
    memshadow_homomorphic_ciphertext_t *ciphertext,
    memshadow_pqc_algorithm_t algorithm
);

/* Cleanup homomorphic ciphertext */
void memshadow_homomorphic_ciphertext_cleanup(
    memshadow_homomorphic_ciphertext_t *ciphertext
);

/* Homomorphic encryption */
memshadow_pqc_error_t memshadow_pqc_he_encrypt(
    memshadow_pqc_manager_t *manager,
    const uint8_t *plaintext,
    size_t plaintext_size,
    memshadow_homomorphic_ciphertext_t *ciphertext,
    const memshadow_hybrid_keypair_t *public_key
);

/* Homomorphic decryption */
memshadow_pqc_error_t memshadow_pqc_he_decrypt(
    memshadow_pqc_manager_t *manager,
    const memshadow_homomorphic_ciphertext_t *ciphertext,
    uint8_t *plaintext,
    size_t *plaintext_size,
    const memshadow_hybrid_keypair_t *private_key
);

/* Homomorphic addition */
memshadow_pqc_error_t memshadow_pqc_he_add(
    memshadow_pqc_manager_t *manager,
    const memshadow_homomorphic_ciphertext_t *ct1,
    const memshadow_homomorphic_ciphertext_t *ct2,
    memshadow_homomorphic_ciphertext_t *result,
    const memshadow_hybrid_keypair_t *evaluation_key
);

/* Homomorphic multiplication */
memshadow_pqc_error_t memshadow_pqc_he_multiply(
    memshadow_pqc_manager_t *manager,
    const memshadow_homomorphic_ciphertext_t *ct1,
    const memshadow_homomorphic_ciphertext_t *ct2,
    memshadow_homomorphic_ciphertext_t *result,
    const memshadow_hybrid_keypair_t *evaluation_key,
    const memshadow_hybrid_keypair_t *relinearization_key
);

/* Homomorphic rotation (CKKS/TFHE) */
memshadow_pqc_error_t memshadow_pqc_he_rotate(
    memshadow_pqc_manager_t *manager,
    const memshadow_homomorphic_ciphertext_t *ciphertext,
    int32_t steps,
    memshadow_homomorphic_ciphertext_t *result,
    const memshadow_hybrid_keypair_t *galois_key
);

/* Check if ciphertext can be operated on */
bool memshadow_homomorphic_ciphertext_can_operate(
    const memshadow_homomorphic_ciphertext_t *ciphertext
);

/* Get error message string */
const char *memshadow_pqc_error_message(memshadow_pqc_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* MEMSHADOW_PQC_H */
