#ifndef UMA_DISPATCHER_H
#define UMA_DISPATCHER_H

#include <cstdint>
#include <cstddef>

extern "C" {
int64_t uma_hybrid_lookup(const int64_t* dataset, size_t size, int64_t query);
}

#endif // UMA_DISPATCHER_H
