#include "../include/keystone_interpolation.h"

extern "C" {
#include "keystone.h"
}

int64_t keystone_scalar_interpolation(const int64_t* dataset, size_t size, int64_t query) {
    keystone_anchor_table_t* table = keystone_anchor_table_create();
    if (!table) return -1;

    keystone_init_for_dsmil(table, 0);

    keystone_result_t result = keystone_search(dataset, size, query, table, 0);

    keystone_anchor_table_destroy(table);

    return (int64_t)result;
}
