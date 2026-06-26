#ifndef QIHSE_RFF_H
#define QIHSE_RFF_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Project a 1D integer dataset into a higher-dimensional float array using
 * Random Fourier Features (RFF).
 * 
 * @param dataset The input 1D integer dataset.
 * @param size The number of elements in the dataset.
 * @param hilbert_space_out The output array, which must have space for 
 *                          (size * 16) floats.
 */
void qihse_hilbert_space_expansion(const int64_t* dataset, size_t size, float* hilbert_space_out);

#ifdef __cplusplus
}
#endif

#endif // QIHSE_RFF_H
