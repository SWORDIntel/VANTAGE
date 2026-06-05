local wezterm = require 'wezterm'
local state = require 'sentinel.state'

local M = {}

-- This function draws the WezTerm native TUI replacement for sentinel_toggles_tui.py
function M.show_sentinel_menu(window, pane)
    -- Read the current state of modules from state.json
    local current_state = state.read_state()
    
    local choices = {}
    -- Example modules we can toggle
    local modules = {
        { id = "ml_enhanced", name = "Enhanced Machine Learning" },
        { id = "secure_mode", name = "Strict Security Mode" },
        { id = "cybersec", name = "CyberSec OSINT Tools" },
        { id = "auto_correction", name = "Auto Command Correction" },
    }

    -- Build the menu choices
    for i, mod in ipairs(modules) do
        local status = current_state[mod.id] and "ON" or "OFF"
        local label = string.format("Toggle %s [Currently: %s]", mod.name, status)
        table.insert(choices, { id = mod.id, label = label })
    end

    table.insert(choices, { id = "cancel", label = "Cancel / Exit Menu" })

    -- Open the native WezTerm overlay menu
    window:perform_action(
        wezterm.action.InputSelector {
            action = wezterm.action_callback(function(inner_window, inner_pane, id, label)
                if not id or id == "cancel" then
                    wezterm.log_info 'SENTINEL menu closed.'
                    return
                end

                -- Toggle the state and write it back to disk
                current_state[id] = not current_state[id]
                state.write_state(current_state)

                local new_status = current_state[id] and "ENABLED" or "DISABLED"
                wezterm.log_info("SENTINEL Module: " .. id .. " is now " .. new_status)
                
                -- Tell WezTerm to emit a toast notification
                inner_window:toast_notification("SENTINEL Control", id .. " is now " .. new_status, nil, 2000)

            end),
            title = 'SENTINEL Master Control',
            choices = choices,
            alphabet = '123456789', -- Allows pressing 1-9 to select options instantly
            description = 'Select a module to toggle. Changes save to state.json instantly.',
        },
        pane
    )
end

function M.setup(config)
    -- We don't need to mutate `config` directly for the UI module,
    -- but this setup function keeps the architecture consistent.
end

return M
