local wezterm = require 'wezterm'

local module = {}

local function get_workspaces()
  local success, stdout, stderr = wezterm.run_child_process({
    "bash", "-c", "ls -d ~/VANTAGE/* ~/.config/* 2>/dev/null"
  })

  local choices = {}
  if success then
    for line in stdout:gmatch("([^\n]+)") do
      -- Remove trailing slash if any
      line = line:gsub("/$", "")
      -- Use the last component of the path as the name/label
      local label = line:match("([^/]+)$") or line
      table.insert(choices, { id = line, label = label })
    end
  else
    wezterm.log_error("Failed to list workspaces: " .. tostring(stderr))
  end

  return choices
end

function module.setup(config)
  config.keys = config.keys or {}

  -- Bind to LEADER + w as an example. You can override or manage this in keys.lua
  table.insert(config.keys, {
    key = 'w',
    mods = 'LEADER',
    action = wezterm.action_callback(function(window, pane)
      local choices = get_workspaces()

      window:perform_action(
        wezterm.action.InputSelector {
          action = wezterm.action_callback(function(inner_window, inner_pane, id, label)
            if id and label then
              inner_window:perform_action(
                wezterm.action.SwitchToWorkspace {
                  name = label,
                  spawn = {
                    cwd = id,
                  },
                },
                inner_pane
              )
            end
          end),
          title = 'Select Workspace',
          choices = choices,
          fuzzy = true,
        },
        pane
      )
    end),
  })
end

return module
