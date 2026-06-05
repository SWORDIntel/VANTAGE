local wezterm = require 'wezterm'
local notifications = require 'sentinel.notifications'

local M = {}
M.checked = false

function M.setup(config)
    wezterm.on('update-status', function(window, pane)
        if M.checked then return end
        M.checked = true
        
        -- Simple Native Lua Healthcheck
        local binaries = {'git', 'curl', 'jq'}
        local missing = {}
        for _, bin in ipairs(binaries) do
            local success, _, _ = wezterm.run_child_process{'which', bin}
            if not success then
                table.insert(missing, bin)
            end
        end
        
        if #missing > 0 then
            notifications.send_toast(window, "Health Check Failed", "Missing core binaries: " .. table.concat(missing, ", "), "CRITICAL")
        else
            notifications.send_toast(window, "SENTINEL Core", "All modules and dependencies online.", "SUCCESS")
        end
    end)
end

return M
