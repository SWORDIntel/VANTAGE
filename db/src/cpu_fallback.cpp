#include "../include/cpu_fallback.h"
#include <iostream>
#include <omp.h>
#include <cmath>
#include <vector>

void execute_cpu_native_fallback(int* dataset, size_t size, int query) {
    if (!dataset || size == 0) return;

    std::cout << "[CPU Fallback] Executing native parallel fallback..." << std::endl;

    std::vector<double> probabilities(size, 1.0 / size);

    // Oracle phase simulation
    #pragma omp parallel for
    for (size_t i = 0; i < size; ++i) {
        if (dataset[i] == query) {
            probabilities[i] = -probabilities[i];
        }
        // Simulate heavy AVX-512/AMX projection workloads
        double dummy = std::sin(dataset[i]) * std::cos(query);
        (void)dummy; // suppress unused variable warning if optimized
    }

    // Grover amplification (diffusion) phase simulation
    double mean = 0.0;
    #pragma omp parallel for reduction(+:mean)
    for (size_t i = 0; i < size; ++i) {
        mean += probabilities[i];
    }
    mean /= size;

    #pragma omp parallel for
    for (size_t i = 0; i < size; ++i) {
        probabilities[i] = 2.0 * mean - probabilities[i];
    }

    std::cout << "[CPU Fallback] Grover amplification phase completed." << std::endl;
}
