local wezterm = require 'wezterm'

local module = {}

function module.show_git_status(window, pane)
  local cwd_uri = pane:get_current_working_dir()
  if not cwd_uri then
    return
  end

  local cwd = cwd_uri
  if type(cwd_uri) == 'userdata' or type(cwd_uri) == 'table' then
    cwd = cwd_uri.file_path
  elseif type(cwd_uri) == 'string' then
    cwd = cwd_uri:gsub('^file://[^/]*', '')
  end

  -- Spawn a background process to run git branch and git status -s
  local success, stdout, stderr = wezterm.run_child_process {
    'bash', '-c',
    'cd "$1" && git branch && echo "---" && git status -s',
    'bash', cwd
  }

  local choices = {}
  if success and stdout then
    for line in stdout:gmatch('[^\r\n]+') do
      table.insert(choices, { label = line })
    end
  else
    table.insert(choices, { label = "Failed to run git or not a git repository" })
  end

  window:perform_action(
    wezterm.action.InputSelector {
      action = wezterm.action_callback(function(win, pn, id, label)
        -- No action specified on selection
      end),
      title = 'Git Status',
      choices = choices,
    },
    pane
  )
end

return module
