local wezterm = require 'wezterm'

local M = {}

-- Spawns a 30-column sidebar on the right dedicated to LLM chat
function M.toggle_chat_sidebar(window, pane)
    local tab = window:active_tab()
    
    -- Check if the sidebar is already open in this tab
    -- A simple heuristic: check if any pane in this tab is running our chat script
    -- For now, we will just blindly split or we can use pane user_vars to track it.
    
    local chat_pane = pane:split {
        direction = 'Right',
        size = 30,
        -- Launch the vantage chat module. We source bashrc so the function is available.
        args = { 'bash', '-c', 'source ~/.bashrc; if type vantage_chat >/dev/null 2>&1; then vantage_chat; else echo "vantage_chat module not found"; sleep 10; fi' },
    }
    
    -- Optional: set a user var so we know this is the chat pane
    -- chat_pane:set_user_var("is_chat_sidebar", "true")
end

-- Spawns the OSINT dashboard (Pillar 3)
function M.launch_osint_dashboard(window, pane)
    local target = window:get_selection_text_for_pane(pane)
    if not target or target == "" then
        -- If nothing highlighted, ask the user
        window:perform_action(
            wezterm.action.PromptInputLine {
                description = 'Enter OSINT Target (IP/Domain):',
                action = wezterm.action_callback(function(inner_window, inner_pane, line)
                    if line then
                        M._do_osint_split(inner_window, inner_pane, line)
                    end
                end),
            },
            pane
        )
    else
        M._do_osint_split(window, pane, target)
    end
end

function M._do_osint_split(window, pane, target)
    wezterm.log_info("Launching OSINT dashboard for: " .. target)
    
    -- Split bottom for nmap
    local bottom_pane = pane:split {
        direction = 'Bottom',
        size = 0.3,
        args = { 'bash', '-c', 'echo "Running NMAP against ' .. target .. '..."; sleep 2; nmap -F ' .. target .. '; exec bash' },
    }
    
    -- Split right for whois
    local right_pane = pane:split {
        direction = 'Right',
        size = 0.5,
        args = { 'bash', '-c', 'echo "Running WHOIS against ' .. target .. '..."; whois ' .. target .. ' | head -n 20; exec bash' },
    }
end

return M
