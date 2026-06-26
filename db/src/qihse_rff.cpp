#include <cstddef>

#include "qihse.h"

// Mock the requested structures and functions safely based on qihse.h.
struct qihse_workload_t {
    const void* data;
    size_t n;
    const void* query;
    qihse_data_type_t type;
};

// Mock qihse_init to boot the heterogeneous engine
static qihse_compute_pool_t* g_compute_pool = nullptr;

void qihse_init() {
    if (!g_compute_pool) {
        g_compute_pool = qihse_compute_pool_init();
    }
}

// Mock qihse_cleanup to cleanly shut down the engine
void qihse_cleanup() {
    if (g_compute_pool) {
        qihse_compute_pool_destroy(g_compute_pool);
        g_compute_pool = nullptr;
    }
}

// Mock qihse_execute_search wrapping the real qihse_search call safely
not_stisla_result_t qihse_execute_search(qihse_workload_t* workload) {
    qihse_config_t config;
    // Initialize config safely based on workload parameters
    qihse_config_init(&config, workload->type, workload->n);
    
    // Perform the actual native QIHSE search
    return qihse_search(workload->data, workload->n, workload->query, nullptr, &config);
}

// Binding pipeline
extern "C" not_stisla_result_t qihse_fallback_pipeline(const void* data, size_t n, const void* query, int type_enum) {
    // Boot heterogeneous engine
    qihse_init();
    
    // Allocate workload
    qihse_workload_t workload;
    workload.data = data;
    workload.n = n;
    workload.query = query;
    workload.type = static_cast<qihse_data_type_t>(type_enum);
    
    // Execute search
    not_stisla_result_t result = qihse_execute_search(&workload);
    
    // Cleanly shut down
    qihse_cleanup();
    
    // Return result
    return result;
}
