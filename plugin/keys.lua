local wezterm = require 'wezterm'
local ui = require 'vantage.ui'
local dashboards = require 'vantage.dashboards'
local chains = require 'vantage.chains'
local git_ui = require 'vantage.git_ui'
local workspace_builder = require 'vantage.workspace_builder'

local M = {}

function M.setup(config)
    if not config.keys then config.keys = {} end

    -- To enable the Leader key for workspace_switcher
    config.leader = { key = 'a', mods = 'CTRL', timeout_milliseconds = 1000 }

    local custom_keys = {
        -- Master VANTAGE UI Toggle
        {
            key = 'M',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(ui.show_vantage_menu),
        },
        -- Pillar 1: LLM Chat Sidebar
        {
            key = 'C',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(dashboards.toggle_chat_sidebar),
        },
        -- Pillar 3: OSINT Target Dashboard
        {
            key = 'O',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(dashboards.launch_osint_dashboard),
        },
        -- Pillar 4: Fuzzy Command Chains
        {
            key = 'F',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(chains.show_command_chains),
        },
        -- Pillar 5: AI Command Suggestions
        {
            key = 'Space',
            mods = 'CTRL',
            action = wezterm.action_callback(chains.ai_suggest),
        },
        -- Pillar 7: Native Git UI Overlay
        {
            key = 'G',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(git_ui.show_git_status),
        },
        -- Pillar 2: Context Sessions (Switch Workspace Native)
        {
            key = 'W',
            mods = 'CTRL|SHIFT',
            action = wezterm.action.ShowLauncherArgs { flags = 'FUZZY|WORKSPACES' },
        },
        -- New Plugin: Workspace Builder Dashboard
        {
            key = 'D',
            mods = 'CTRL|SHIFT',
            action = wezterm.action_callback(workspace_builder.build_dashboard),
        },
    }

    for _, k in ipairs(custom_keys) do
        table.insert(config.keys, k)
    end
end

return M
