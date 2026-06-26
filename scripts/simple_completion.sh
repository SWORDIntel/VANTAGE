#!/usr/bin/env bash
# Enhanced simple and safe completion for VANTAGE with BLE.sh integration attempt
# This script is designed to be loaded directly from .bashrc and provides a safe
# fallback when the full autocomplete module is disabled

# Print status message to confirm execution
echo "Simple completion script executing..."

# Prevent double loading with robust checks
[[ -n "${_VANTAGE_SIMPLE_COMPLETION_LOADED}" ]] && { echo "Simple completion already loaded"; return 0; }
export _VANTAGE_SIMPLE_COMPLETION_LOADED=1

# Create required directories with error handling
{ mkdir -p "$HOME/autocomplete/completions"; } 2>/dev/null || true

# Try to load BLE.sh first, but continue with simple completion if it fails
if [[ -f "$HOME/blesh_loader.sh" ]]; then
  # First ensure we have the correct attachment method
  export BLESH_ATTACH_METHOD="attach"
  # Try to source with robust error handling
  if { source "$HOME/blesh_loader.sh"; } 2>/dev/null; then
    echo "BLE.sh loaded successfully by simple completion system"
  else
    echo "Falling back to native bash completion"
  fi
fi

# Load standard bash completions safely
echo "Loading basic bash completion (native)..."
if [[ -f /etc/bash_completion ]]; then
  { source /etc/bash_completion; } 2>/dev/null || true
fi

# Enable programmable completion
{ shopt -s progcomp; } 2>/dev/null || true

# Load custom completions if they exist
if [[ -d "$HOME/autocomplete/completions" ]]; then
  for f in "$HOME/autocomplete/completions"/*.completion; do
    if [[ -f "$f" ]]; then
      { source "$f"; } 2>/dev/null || true
    fi
  done
fi

# Enhanced custom tab completion for common VANTAGE and system commands
_vantage_simple_complete() {
  local cur prev opts
  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"
  
  # Special case for initial command completion
  if [[ ${COMP_CWORD} -eq 0 ]]; then
    COMPREPLY=($(compgen -c -- "${cur}"))
    return 0
  fi
  
  # VANTAGE specific commands
  if [[ "$prev" == "@"* ]]; then
    local vantage_opts="help status fix reload install modules"
    COMPREPLY=( $(compgen -W "${vantage_opts}" -- ${cur}) )
    return 0
  fi
  
  # Common Linux commands completion
  case "${prev}" in
    cd)
      local dirs=$(ls -d */ 2>/dev/null)
      COMPREPLY=( $(compgen -W "${dirs}" -- ${cur}) )
      return 0
      ;;
    git)
      local git_opts="pull push status checkout branch commit merge rebase log clone fetch diff stash"
      COMPREPLY=( $(compgen -W "${git_opts}" -- ${cur}) )
      return 0
      ;;
    ls|ll|la)
      local dirs=$(ls -A 2>/dev/null)
      COMPREPLY=( $(compgen -W "${dirs}" -- ${cur}) )
      return 0
      ;;
    ssh)
      # Try to parse known hosts file for completions
      if [[ -f ~/.ssh/known_hosts ]]; then
        local hosts=$(awk '{print $1}' ~/.ssh/known_hosts 2>/dev/null | tr ',' '\n' | sed 's/\[//g' | sed 's/\]//g' | cut -d: -f1 | sort -u)
        COMPREPLY=( $(compgen -W "${hosts}" -- ${cur}) )
      fi
      return 0
      ;;
  esac
  
  # Default filename completion for other commands with error handling
  return 0
}

# Register completion function for common commands
{ complete -o default -F _vantage_simple_complete; } 2>/dev/null || true

# Add some specific command completions with error handling
{ complete -o default -F _vantage_simple_complete git; } 2>/dev/null || true
{ complete -o default -F _vantage_simple_complete ls; } 2>/dev/null || true
{ complete -o default -F _vantage_simple_complete cd; } 2>/dev/null || true

# Always return success
return 0
