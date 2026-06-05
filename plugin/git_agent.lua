local wezterm = require 'wezterm'

local module = {}

function module.apply_to_config(config)
  if not config.keys then
    config.keys = {}
  end

  table.insert(config.keys, {
    key = 'C',
    mods = 'CTRL|SHIFT',
    action = wezterm.action_callback(function(window, pane)
      local cwd_uri = pane:get_current_working_dir()
      local cwd = cwd_uri and cwd_uri.file_path
      if not cwd then
        wezterm.log_error("Git Agent: Could not determine current working directory.")
        return
      end

      -- Try staged diff first
      local success, stdout, stderr = wezterm.run_child_process({"git", "-C", cwd, "diff", "--staged"})
      if not success or not stdout or stdout:match("^%s*$") then
        -- Fallback to unstaged diff
        success, stdout, stderr = wezterm.run_child_process({"git", "-C", cwd, "diff"})
      end

      if not success or not stdout or stdout:match("^%s*$") then
        wezterm.log_info("Git Agent: No git diff output found.")
        return
      end

      local prompt = "Based on the following git diff, generate a concise conventional commit message. Return ONLY the commit message, without any markdown formatting, quotes, or additional text.\n\nDiff:\n" .. stdout

      local gpu_type = (wezterm.run_child_process{"nvidia-smi"}) and "nvidia" or "cpu"
      local use_framewerx = false
      local sentinel_path = os.getenv("HOME") .. "/SENTINEL"
      
      local f = io.open(sentinel_path .. "/framewerx_engine/fw_launcher.py", "r")
      if f then
          f:close()
          use_framewerx = true
      end
      local fw_native_dir = io.open(sentinel_path .. "/framewerx_engine/native/", "r")
      if fw_native_dir then
          fw_native_dir:close()
          use_framewerx = true
      end

      local suggestions_text = nil

      if gpu_type == "cpu" and use_framewerx then
          wezterm.log_info("Git Agent: Using TurboQuant via Framewerx fallback")
          local fw_success, fw_stdout, fw_stderr = wezterm.run_child_process {
              "python3",
              sentinel_path .. "/framewerx_engine/fw_launcher.py",
              "--prompt",
              prompt
          }
          if fw_success and fw_stdout and fw_stdout ~= "" then
              suggestions_text = fw_stdout
          else
              wezterm.log_info("Git Agent: Framewerx failed, falling back to Ollama. Error: " .. tostring(fw_stderr))
              use_framewerx = false
          end
      end

      if not suggestions_text then
          local model = (gpu_type == "nvidia") and "llama3" or "codellama"
          wezterm.log_info("Git Agent: Using Ollama (" .. model .. ")")
          
          local escape_json = function(s)
            return s:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
          end

          local json_payload = string.format('{"model": "%s", "prompt": "%s", "stream": false, "options": {"num_predict": 50, "num_ctx": 2048, "num_gpu": %d}}', 
              model, 
              escape_json(prompt), 
              gpu_type == "nvidia" and 99 or 0
          )
          
          local ollama_success, ollama_stdout, ollama_stderr = wezterm.run_child_process {
            "curl",
            "-s",
            "-X", "POST",
            "http://localhost:11434/api/generate",
            "-H", "Content-Type: application/json",
            "-d", json_payload
          }
          
          if ollama_success then
            local success_json, parsed = pcall(wezterm.json_parse, ollama_stdout)
            if success_json and type(parsed) == "table" and parsed.response then
              suggestions_text = parsed.response
            else
              wezterm.log_error("Git Agent: Failed to parse JSON or missing response.")
            end
          else
            wezterm.log_error("Git Agent: Failed to run curl. " .. (ollama_stderr or ""))
          end
      end

      if suggestions_text then
          -- Clean up whitespace, backticks, quotes
          local clean_msg = suggestions_text:gsub("^%s*", ""):gsub("%s*$", ""):gsub('^"', ''):gsub('"$', ''):gsub('^`+', ''):gsub('`+$', '')
          if clean_msg ~= "" then
              local escaped_msg = clean_msg:gsub('"', '\\"')
              pane:send_text('git commit -m "' .. escaped_msg .. '"')
          else
              wezterm.log_error("Git Agent: Generated commit message is empty.")
          end
      else
          wezterm.log_error("Git Agent: No suggestions returned.")
      end
    end)
  })
end

return module
