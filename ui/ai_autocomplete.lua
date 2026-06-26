local wezterm = require 'wezterm'

local module = {}

function module.apply_to_config(config)
  if not config.keys then
    config.keys = {}
  end

  table.insert(config.keys, {
    key = 'Space',
    mods = 'CTRL',
    action = wezterm.action_callback(function(window, pane)
      -- Extract the current pane's text
      local text = pane:get_lines_as_text(150)
      if not text then
        text = pane:get_lines_as_text()
      end
      
      -- Detect foreground process to use as context
      local fg_process = pane:get_foreground_process_name() or "unknown process"
      
      -- Detect GPU (using nvidia-smi as a simple check for dedicated NVIDIA GPU)
      local success_nvidia = wezterm.run_child_process{"nvidia-smi"}
      local gpu_type = success_nvidia and "nvidia" or "cpu"
      
      -- Check for Framewerx (TurboQuant CPU/VPU inference) fallback
      local use_framewerx = false
      local f = io.open("/home/john/Documents/framewerx/fw_launcher.py", "r")
      if f then
          f:close()
          use_framewerx = true
      end
      
      local fw_native_dir = io.open("/home/john/Documents/framewerx/native/", "r")
      if fw_native_dir then
          fw_native_dir:close()
          use_framewerx = true
      end
      
      -- Prepare the prompt asking for shell command suggestions
      local prompt = string.format("Current running process: %s\nBased on the following terminal output, suggest 3 shell commands that the user might want to run next. Return ONLY the commands, one per line, with no extra text or numbering.\n\nOutput:\n%s", fg_process, text)
      
      -- Simple function to escape strings for JSON
      local escape_json = function(s)
        return s:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
      end
      
      local suggestions_text = nil
      local title_suffix = ""

      -- Attempt to use Framewerx fallback if no dedicated GPU
      if gpu_type == "cpu" and use_framewerx then
          wezterm.log_info("AI Autocomplete: Using TurboQuant via Framewerx fallback")
          title_suffix = " (TurboQuant CPU/VPU)"
          local fw_success, fw_stdout, fw_stderr = wezterm.run_child_process {
              "python3",
              "/home/john/Documents/framewerx/fw_launcher.py",
              "--prompt",
              prompt
          }
          if fw_success and fw_stdout and fw_stdout ~= "" then
              suggestions_text = fw_stdout
          else
              wezterm.log_info("Framewerx failed or returned empty, falling back to local Ollama API. Error: " .. tostring(fw_stderr))
              use_framewerx = false -- trigger Ollama fallback
          end
      end
      
      -- Primary / Fallback: Local Ollama API
      if not suggestions_text then
          -- Dynamically select model based on hardware
          local model = (gpu_type == "nvidia") and "llama3" or "codellama"
          title_suffix = (gpu_type == "nvidia") and " (TurboQuant GPU KV Cache)" or " (Ollama CPU)"
          wezterm.log_info("AI Autocomplete: Using Ollama (" .. model .. ")" .. title_suffix)
          
          -- Configure KV Cache tech parameters
          local json_payload = string.format('{"model": "%s", "prompt": "%s", "stream": false, "options": {"num_predict": 50, "num_ctx": 2048, "num_gpu": %d}}', 
              model, 
              escape_json(prompt), 
              gpu_type == "nvidia" and 99 or 0
          )
          
          -- Spawn a child process to call the local Ollama API
          local success, stdout, stderr = wezterm.run_child_process {
            "curl",
            "-s",
            "-X", "POST",
            "http://localhost:11434/api/generate",
            "-H", "Content-Type: application/json",
            "-d", json_payload
          }
          
          if success then
            -- Parse the JSON response
            local success_json, parsed = pcall(wezterm.json_parse, stdout)
            if success_json and type(parsed) == "table" and parsed.response then
              suggestions_text = parsed.response
            else
              wezterm.log_error("AI Autocomplete Error: Failed to parse JSON or missing response.")
            end
          else
            wezterm.log_error("AI Autocomplete Error: Failed to run curl. " .. (stderr or ""))
          end
      end
      
      if suggestions_text then
        -- Split the suggestions into choices for InputSelector
        local choices = {}
        for line in suggestions_text:gmatch("[^\r\n]+") do
          -- Clean up typical markdown or numbering artifacts
          local clean_line = line:gsub("^%s*[%-%*%d%.]*%s*", ""):gsub("^`+", ""):gsub("`+$", "")
          if clean_line ~= "" then
            table.insert(choices, { label = clean_line })
          end
        end
        
        -- Use WezTerm's InputSelector to display them
        if #choices > 0 then
          window:perform_action(
            wezterm.action.InputSelector {
              action = wezterm.action_callback(function(inner_window, inner_pane, id, label)
                if not id and not label then
                  wezterm.log_info('AI Autocomplete: cancelled')
                else
                  -- Inject the selected suggestion into the pane
                  inner_pane:send_text(label)
                end
              end),
              title = 'AI Autocomplete Suggestions' .. title_suffix,
              choices = choices,
            },
            pane
          )
        else
          wezterm.log_error("AI Autocomplete: No suggestions returned.")
        end
      end
    end)
  })
end

return module
