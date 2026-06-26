#include "../include/uma_dispatcher.h"
#include "../include/keystone_interpolation.h"
#include <iostream>

// Mock for QIHSE fallback
int64_t qihse_fallback_pipeline(const int64_t* dataset, size_t size, int64_t query) {
    (void)dataset;
    (void)size;
    (void)query;
    std::cout << "[QIHSE] Executing QIHSE fallback pipeline...\n";
    return query; // Stub
}

extern "C" int64_t uma_hybrid_lookup(const int64_t* dataset, size_t size, int64_t query) {
    // Simulate a skew/density check
    // Arbitrary logic for simulation: blocks > 1024 elements are considered dense/skewed
    bool is_dense_or_skewed = (size > 1024);
    
    if (is_dense_or_skewed) {
        std::cout << "[UMA] Data block is dense/skewed. Calling QIHSE fallback.\n";
        return qihse_fallback_pipeline(dataset, size, query);
    } else {
        std::cout << "[UMA] Data block is linear/sparse. Calling Keystone scalar interpolation.\n";
        return keystone_scalar_interpolation(dataset, size, query);
    }
}
