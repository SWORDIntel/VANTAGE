local wezterm = require('wezterm')

local M = {}

local SOLID_LEFT_ARROW = ''
local SOLID_RIGHT_ARROW = ''

-- Palette inspired by modern themes
local colors = {
  tab_bar_bg = '#1e1e2e',
  active_bg = '#cba6f7',
  active_fg = '#11111b',
  inactive_bg = '#313244',
  inactive_fg = '#cdd6f4',
  hover_bg = '#45475a',
  hover_fg = '#cdd6f4',
  workspace_bg = '#a6e3a1',
  workspace_fg = '#11111b',
}

function M.setup(config)
  -- Customizing the tab bar strictly requires the retro tab bar
  config.use_fancy_tab_bar = false
  config.tab_bar_at_bottom = true
  config.show_new_tab_button_in_tab_bar = false
  
  -- Default colors for the tab bar background
  config.colors = config.colors or {}
  config.colors.tab_bar = {
    background = colors.tab_bar_bg,
  }

  wezterm.on('format-tab-title', function(tab, tabs, panes, conf, hover, max_width)
    local background = colors.inactive_bg
    local foreground = colors.inactive_fg

    if tab.is_active then
      background = colors.active_bg
      foreground = colors.active_fg
    elseif hover then
      background = colors.hover_bg
      foreground = colors.hover_fg
    end

    local title = string.format(' %d: %s ', tab.tab_index + 1, tab.active_pane.title)

    return {
      { Background = { Color = background } },
      { Foreground = { Color = foreground } },
      { Text = title },
      { Background = { Color = colors.tab_bar_bg } },
      { Foreground = { Color = background } },
      { Text = SOLID_RIGHT_ARROW },
    }
  end)

  wezterm.on('update-status', function(window, pane)
    local workspace = window:active_workspace()

    -- Display the workspace name on the right side of the status bar
    window:set_right_status(wezterm.format({
      { Background = { Color = colors.tab_bar_bg } },
      { Foreground = { Color = colors.workspace_bg } },
      { Text = SOLID_LEFT_ARROW },
      { Background = { Color = colors.workspace_bg } },
      { Foreground = { Color = colors.workspace_fg } },
      { Attribute = { Intensity = 'Bold' } },
      { Text = string.format(' 󱂬 %s ', workspace) },
    }))
  end)
end

return M
