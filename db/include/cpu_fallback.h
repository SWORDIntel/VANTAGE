#ifndef HYBRID_DB_CPU_FALLBACK_H
#define HYBRID_DB_CPU_FALLBACK_H

#include <cstddef>

void execute_cpu_native_fallback(int* dataset, size_t size, int query);

#endif // HYBRID_DB_CPU_FALLBACK_H
