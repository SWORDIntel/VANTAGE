local wezterm = require 'wezterm'
local vantage = wezterm.plugin.require('https://github.com/SWORDIntel/VANTAGE')

local config = wezterm.config_builder()
vantage.apply_to_config(config)

return config
