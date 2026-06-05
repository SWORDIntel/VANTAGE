local wezterm = require 'wezterm'
local io = require 'io'
local os = require 'os'
local M = {}

local state_path = os.getenv("HOME") .. "/.config/sentinel/state.json"

function M.read_state()
  local file = io.open(state_path, "r")
  if not file then return {} end
  
  local content = file:read("*a")
  file:close()
  
  if content and content ~= "" then
    if wezterm.json_parse then
      local success, parsed = pcall(wezterm.json_parse, content)
      if success then return parsed end
    elseif wezterm.serde and wezterm.serde.json_decode then
      local success, parsed = pcall(wezterm.serde.json_decode, content)
      if success then return parsed end
    end
  end
  return {}
end

function M.write_state(state)
  local file = io.open(state_path, "w")
  if not file then return false end
  
  local encoded = "{}"
  if wezterm.json_encode then
    local success, enc = pcall(wezterm.json_encode, state)
    if success then encoded = enc end
  elseif wezterm.serde and wezterm.serde.json_encode then
    local success, enc = pcall(wezterm.serde.json_encode, state)
    if success then encoded = enc end
  end
  
  file:write(encoded)
  file:close()
  return true
end

return M
