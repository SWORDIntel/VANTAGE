local wezterm = require 'wezterm'
local M = {}

function M.apply_to_config(config)
    local frameworks_dir = os.getenv("HOME") .. "/SENTINEL/plugin/frameworks"
    -- Look for directories containing sentinel_hook.lua
    local search_path = frameworks_dir .. "/*/sentinel_hook.lua"
    local handle = io.popen('ls ' .. search_path .. ' 2>/dev/null')
    
    if handle then
        local files = handle:read("*a")
        handle:close()

        for path in files:gmatch("[^\r\n]+") do
            local chunk, err = loadfile(path)
            if chunk then
                local ok, mod = pcall(chunk)
                if ok and type(mod) == "table" and type(mod.apply_to_config) == "function" then
                    local apply_ok, apply_err = pcall(mod.apply_to_config, config)
                    if not apply_ok then
                        wezterm.log_error("SENTINEL: Error applying config for framework hook " .. path .. ": " .. tostring(apply_err))
                    end
                end
            else
                wezterm.log_error("SENTINEL: Failed to load framework hook " .. path .. ": " .. tostring(err))
            end
        end
    end
end

return M
