local env = require 'sentinel.env'
local keys = require 'sentinel.keys'
local ui = require 'sentinel.ui'
local background = require 'sentinel.background'
local workspaces = require 'sentinel.workspaces'
local resurrect = require 'sentinel.resurrect'
local workspace_switcher = require 'sentinel.workspace_switcher'
local workspace_builder = require 'sentinel.workspace_builder'
local tabline = require 'sentinel.tabline'
local smooth_scroll = require 'sentinel.smooth_scroll'
local notifications = require 'sentinel.notifications'
local health = require 'sentinel.health'
local ai_autocomplete = require 'sentinel.ai_autocomplete'

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
end

return M
