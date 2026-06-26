#ifndef KEYSTONE_INTERPOLATION_H
#define KEYSTONE_INTERPOLATION_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

int64_t keystone_scalar_interpolation(const int64_t* dataset, size_t size, int64_t query);

#ifdef __cplusplus
}
#endif

#endif // KEYSTONE_INTERPOLATION_H
