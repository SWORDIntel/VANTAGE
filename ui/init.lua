local env = require 'vantage.env'
local keys = require 'vantage.keys'
local ui = require 'vantage.ui'
local background = require 'vantage.background'
local workspaces = require 'vantage.workspaces'
local resurrect = require 'vantage.resurrect'
local workspace_switcher = require 'vantage.workspace_switcher'
local workspace_builder = require 'vantage.workspace_builder'
local tabline = require 'vantage.tabline'
local smooth_scroll = require 'vantage.smooth_scroll'
local notifications = require 'vantage.notifications'
local health = require 'vantage.health'
local ai_autocomplete = require 'vantage.ai_autocomplete'
local framework_loader = dofile(os.getenv("HOME") .. "/VANTAGE/plugin/framework_loader.lua")
local git_agent = require 'vantage.git_agent'
local ssh_injector = require 'vantage.ssh_injector'

local M = {}

function M.apply_to_config(config)
    env.setup(config)
    keys.setup(config)
    ui.setup(config)
    background.setup(config)
    workspaces.setup(config)
    resurrect.setup(config)
    workspace_switcher.setup(config)
    workspace_builder.setup(config)
    tabline.setup(config)
    smooth_scroll.setup(config)
    notifications.setup(config)
    health.setup(config)
    ai_autocomplete.apply_to_config(config)
    framework_loader.apply_to_config(config)
    git_agent.apply_to_config(config)
    ssh_injector.apply_to_config(config)
end

return M
