local wezterm = require 'wezterm'

local module = {}

function module.setup(config)
  if not config.keys then
    config.keys = {}
  end

  local custom_keys = {
    {
      key = 'PageUp',
      mods = 'CTRL',
      action = wezterm.action.ScrollByLine(-5),
    },
    {
      key = 'PageDown',
      mods = 'CTRL',
      action = wezterm.action.ScrollByLine(5),
    },
  }

  for _, key in ipairs(custom_keys) do
    table.insert(config.keys, key)
  end
end

return module
