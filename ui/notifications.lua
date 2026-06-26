local wezterm = require 'wezterm'

local M = {}

local level_emojis = {
    INFO = "ℹ️ [INFO]",
    SUCCESS = "✅ [SUCCESS]",
    WARNING = "⚠️ [WARNING]",
    CRITICAL = "🚨 [CRITICAL]",
    ERROR = "❌ [ERROR]"
}

function M.send_toast(window, title, message, level)
    local prefix = level_emojis[level] or string.format("🔔 [%s]", level or "INFO")
    local formatted_title = string.format("%s %s", prefix, title)
    
    window:toast_notification(formatted_title, message, nil, 4000)
end

function M.setup(config)
    -- Setup notifications config if necessary
end

return M
