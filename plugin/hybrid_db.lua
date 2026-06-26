local ffi = require("ffi")

ffi.cdef[[
    void qihse_init();
    int64_t qihse_search(...);
    double keystone_scalar_interpolation(...);
]]

local qihse_lib = ffi.load("/home/john/QIHSE/qihse/libqihse.so")

local hybrid_db = {}

function hybrid_db.search_logs(query)
    qihse_lib.qihse_init()
    
    -- Call the loaded C function natively from Lua
    local result = qihse_lib.qihse_search(query)
    
    return tonumber(result)
end

return hybrid_db
