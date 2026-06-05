local wezterm = require 'wezterm'

local module = {}

function module.setup(config)
  wezterm.on('update-status', function(window, pane)
    local workspace = window:active_workspace()
    local overrides = window:get_config_overrides() or {}
    
    local is_alert_workspace = (workspace == 'PenTest' or workspace == 'AWS')
    local is_alert_active = overrides.colors and overrides.colors.tab_bar and overrides.colors.tab_bar.background == '#8b0000'

    if is_alert_workspace and not is_alert_active then
      overrides.colors = {
        tab_bar = {
          background = '#8b0000',
          active_tab = {
            bg_color = '#ff0000',
            fg_color = '#ffffff',
            intensity = 'Bold',
          },
          inactive_tab = {
            bg_color = '#8b0000',
            fg_color = '#cccccc',
          },
          inactive_tab_hover = {
            bg_color = '#aa0000',
            fg_color = '#ffffff',
          },
          new_tab = {
            bg_color = '#8b0000',
            fg_color = '#cccccc',
          },
          new_tab_hover = {
            bg_color = '#aa0000',
            fg_color = '#ffffff',
          },
        }
      }
      window:set_config_overrides(overrides)
    elseif not is_alert_workspace and is_alert_active then
      overrides.colors = nil
      window:set_config_overrides(overrides)
    end
  end)
end

return module
