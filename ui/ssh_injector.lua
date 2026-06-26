local wezterm = require 'wezterm'

local M = {}

function M.apply_to_config(config)
    if not config.keys then
        config.keys = {}
    end

    table.insert(config.keys, {
        key = 'S',
        mods = 'CTRL|SHIFT',
        action = wezterm.action.PromptInputLine {
            description = 'Vantage SSH (Enter Host):',
            action = wezterm.action_callback(function(window, pane, line)
                if line and line ~= '' then
                    local cmd = string.format(
                        "scp -q -r ~/.config/vantage %s:/tmp/vantage_payload 2>/dev/null && ssh -t %s 'bash --rcfile /tmp/vantage_payload/thin_bashrc'",
                        line, line
                    )
                    window:perform_action(
                        wezterm.action.SpawnCommandInNewTab {
                            args = { 'bash', '-c', cmd },
                        },
                        pane
                    )
                end
            end),
        },
    })
end

return M
