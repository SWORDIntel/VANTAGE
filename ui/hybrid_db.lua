local ffi = require("ffi")

ffi.cdef[[
    -- NOT_STISLA anchor search
    typedef struct not_stisla_anchor_table not_stisla_anchor_table_t;
    not_stisla_anchor_table_t* not_stisla_anchor_table_create(void);
    void not_stisla_anchor_table_destroy(not_stisla_anchor_table_t* table);
    size_t not_stisla_search(const int64_t* arr, size_t n, int64_t key,
                             not_stisla_anchor_table_t* table, size_t tol);
    uint32_t not_stisla_detect_cpu_features(void);
    const char* not_stisla_version(void);

    -- QIHSE KV store
    typedef struct qihse_kv_store qihse_kv_store_t;
    qihse_kv_store_t* qihse_kv_store_create(void);
    void qihse_kv_store_destroy(qihse_kv_store_t* store);
    bool qihse_kv_set(qihse_kv_store_t* store, const char* key, const char* value,
                      uint16_t classification, uint16_t sci_compartment);
    char* qihse_kv_get(qihse_kv_store_t* store, const char* key, void* user);
    bool qihse_kv_del(qihse_kv_store_t* store, const char* key, void* user);
]]

local qihse_lib = ffi.load(os.getenv("VANTAGE_QIHSE_LIB") or debug.getinfo(1, "S").source:match("@(.*/)") .. "../vendor/QIHSE/libqihse.so")

local hybrid_db = {}

function hybrid_db.silicon_info()
    local flags = qihse_lib.not_stisla_detect_cpu_features()
    local parts = {}
    if bit.band(flags, 1) ~= 0 then table.insert(parts, "AVX2") end
    if bit.band(flags, 2) ~= 0 then table.insert(parts, "AVX-512") end
    if bit.band(flags, 4) ~= 0 then table.insert(parts, "AMX") end
    if bit.band(flags, 8) ~= 0 then table.insert(parts, "AVX-VNNI") end
    if #parts == 0 then return "scalar-only" end
    return table.concat(parts, " + ")
end

function hybrid_db.search_logs(dataset, query)
    local n = #dataset
    local arr = ffi.new("int64_t[?]", n, dataset)
    local table = qihse_lib.not_stisla_anchor_table_create()
    local idx = qihse_lib.not_stisla_search(arr, n, query, table, 0)
    qihse_lib.not_stisla_anchor_table_destroy(table)
    return tonumber(idx)
end

function hybrid_db.kv_set(key, value)
    local store = qihse_lib.qihse_kv_store_create()
    local ok = qihse_lib.qihse_kv_set(store, key, value, 0, 0)
    qihse_lib.qihse_kv_store_destroy(store)
    return ok
end

function hybrid_db.kv_get(key)
    local store = qihse_lib.qihse_kv_store_create()
    local val = qihse_lib.qihse_kv_get(store, key, nil)
    qihse_lib.qihse_kv_store_destroy(store)
    if val == nil then return nil end
    local result = ffi.string(val)
    ffi.C.free(val)
    return result
end

function hybrid_db.version()
    local v = qihse_lib.not_stisla_version()
    if v == nil then return "unknown" end
    return ffi.string(v)
end

return hybrid_db
