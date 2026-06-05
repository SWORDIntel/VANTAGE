local wezterm = require 'wezterm'
local sentinel = require 'sentinel.init'

local config = wezterm.config_builder()
sentinel.setup(config)

return config
