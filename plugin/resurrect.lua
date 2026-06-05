local wezterm = require("wezterm")
local resurrect = wezterm.plugin.require("file:///home/john/.config/wezterm/plugins/resurrect.wezterm")

-- Configure resurrect to save window state and workspaces
resurrect.state_manager.periodic_save({
  save_workspaces = true,
  save_windows = true,
})

local module = {}

function module.setup(config)
  if not config.keys then
    config.keys = {}
  end

  -- Bind CTRL+SHIFT+S (Save)
  table.insert(config.keys, {
    key = "S",
    mods = "CTRL|SHIFT",
    action = wezterm.action_callback(function(win, pane)
      resurrect.state_manager.save_state(resurrect.workspace_state.get_workspace_state())
      resurrect.window_state.save_window_action()
    end),
  })

  -- Bind CTRL+SHIFT+L (Load)
  table.insert(config.keys, {
    key = "L",
    mods = "CTRL|SHIFT",
    action = wezterm.action_callback(function(win, pane)
      resurrect.fuzzy_loader.fuzzy_load(win, pane, function(id, label)
        local type = string.match(id, "^([^/]+)") -- match before '/'
        id = string.match(id, "([^/]+)$") -- match after '/'
        id = string.match(id, "(.+)%..+$") -- remove file extention
        
        local opts = {
          relative = true,
          restore_text = true,
          on_pane_restore = resurrect.tab_state.default_on_pane_restore,
        }
        
        if type == "workspace" then
          local state = resurrect.state_manager.load_state(id, "workspace")
          resurrect.workspace_state.restore_workspace(state, opts)
        elseif type == "window" then
          local state = resurrect.state_manager.load_state(id, "window")
          resurrect.window_state.restore_window(pane:window(), state, opts)
        elseif type == "tab" then
          local state = resurrect.state_manager.load_state(id, "tab")
          resurrect.tab_state.restore_tab(pane:tab(), state, opts)
        end
      end)
    end),
  })
end

return module
