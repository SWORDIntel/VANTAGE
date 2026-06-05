local wezterm = require 'wezterm'
local sentinel = wezterm.plugin.require('https://github.com/SWORDIntel/SENTINEL')

local config = wezterm.config_builder()
sentinel.apply_to_config(config)

return config
