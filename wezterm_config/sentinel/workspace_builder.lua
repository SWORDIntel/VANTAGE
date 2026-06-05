local wezterm = require 'wezterm'
local module = {}

function module.build_dashboard(window, pane)
  -- Create a right split (top-right)
  local top_right_pane = pane:split {
    direction = 'Right',
    args = {'htop'},
  }
  
  -- Create a bottom split on the original pane (bottom-left)
  local bottom_left_pane = pane:split {
    direction = 'Bottom',
  }

  -- Create a bottom split on the right pane (bottom-right)
  local bottom_right_pane = top_right_pane:split {
    direction = 'Bottom',
    args = {'tail', '-f', '/var/log/syslog'},
  }
end

function module.setup(config)
  -- Module setup function, no config modification required
end

return module
