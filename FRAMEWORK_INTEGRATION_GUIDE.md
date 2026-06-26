# VANTAGE Framework Integration Guide

The Modular Framework Integration Architecture in VANTAGE allows developers to dynamically extend its capabilities. Since frameworks (like Metasploit, Covenant, or Framewerx) are typically full repositories rather than single scripts, VANTAGE uses a bridge-file approach to load external capabilities.

## 1. Adding a New Framework Repository

To add a new integration, clone or place the entire framework repository into the `~/VANTAGE/plugin/frameworks/` directory.

```bash
cd ~/VANTAGE/plugin/frameworks/
git clone https://github.com/example/covenant-vantage.git
```

### The `vantage_hook.lua` Bridge Script

For VANTAGE to recognize and load your framework, your repository **must** contain a `vantage_hook.lua` file in its root directory. This script acts as the bridge between your repository's internal logic, WezTerm's UI, the Abstract Syntax Tree (AST), and the Rust `vantage-core` backend.

The `vantage_hook.lua` file must return a module table containing an `apply_to_config(config)` function.

Example (`~/VANTAGE/plugin/frameworks/covenant-vantage/vantage_hook.lua`):
```lua
local wezterm = require 'wezterm'

-- To load other Lua files from your repository, construct the absolute path
-- or append your repository to package.path
local repo_path = os.getenv("HOME") .. "/VANTAGE/plugin/frameworks/covenant-vantage/"
-- local my_internal_module = dofile(repo_path .. "src/internal.lua")

local M = {}

function M.apply_to_config(config)
    wezterm.log_info("Covenant repository framework hook loaded!")
    
    -- Setup your integration logic here
end

return M
```

When WezTerm starts, `framework_loader.lua` scans all subdirectories in `frameworks/` for `vantage_hook.lua` files and safely executes their `apply_to_config(config)` functions.

## 2. Hooking into WezTerm UI Elements

Your hook script can seamlessly integrate into the WezTerm UI by modifying the `config` object or utilizing WezTerm's event system.

### Custom Menus and Keybinds
You can register custom key bindings to trigger framework-specific actions or open menus using WezTerm's `InputSelector`.

```lua
local wezterm = require 'wezterm'
local act = wezterm.action

function M.apply_to_config(config)
    table.insert(config.keys, {
        key = 'm',
        mods = 'LEADER',
        action = act.InputSelector {
            title = 'Framework Actions',
            choices = {
                { label = 'Start Listener', id = 'start_listener' },
                { label = 'Run Exploit', id = 'run_exploit' },
            },
            action = wezterm.action_callback(function(window, pane, id, label)
                if id then
                    window:toast_notification("Framework", "Executing: " .. label, nil, 4000)
                    -- Trigger appropriate background task or command based on 'id'
                end
            end),
        }
    })
end
```

### Tabs and Panes
You can programmatically spawn new tabs, panes, or windows to house framework interfaces (e.g., CLI tools or dashboards).

```lua
wezterm.on('open-framework-dashboard', function(window, pane)
    window:perform_action(wezterm.action.SpawnCommandInNewTab {
        args = { 'framework-cli', '--dashboard' },
    }, pane)
end)
```

## 3. Interfacing with Rust `vantage-core` and AST

The core engine of VANTAGE is written in Rust (`vantage-core`). Integrations can communicate with `vantage-core` using child processes, WezTerm's background task APIs, named pipes, or UNIX sockets.

### Spawning Child Processes
You can invoke the `vantage-core` CLI directly from your hook module to fetch data, trigger internal state changes, or pass AST payloads.

```lua
local wezterm = require 'wezterm'

function M.apply_to_config(config)
    wezterm.on('fetch-framework-agents', function(window, pane)
        -- Call vantage-core as a child process
        local success, stdout, stderr = wezterm.run_child_process {
            'vantage-core', 'framework', 'list-agents'
        }
        
        if success then
            window:toast_notification("Sentinel Core", "Agents: " .. stdout, nil, 4000)
        end
    end)
end
```

### Advanced IPC (Unix Sockets & Named Pipes)
For robust communication (e.g., streaming real-time events from Framewerx, managing a Covenant listener, or pushing live AST updates), `vantage-core` exposes local UNIX sockets. Your Lua script can interact with this via `socat` or `curl` inside child processes, or by using WezTerm's native plugin ecosystem if socket communication primitives are exposed.

## Important Notes

*   **Initialization Order**: Hook scripts are loaded *after* all core plugins. You can rely on core VANTAGE services being fully initialized.
*   **Error Handling**: The `framework_loader` wraps hook execution in a `pcall`. If a hook throws an error, it is logged to the WezTerm console without crashing VANTAGE.
*   **Module Pathing**: Since your framework is an external repository, standard `require` paths might not point to your internal modules. Use `dofile` with absolute paths, or explicitly update `package.path` inside your `vantage_hook.lua`.
