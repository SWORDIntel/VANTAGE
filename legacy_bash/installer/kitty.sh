#!/usr/bin/env bash
# SENTINEL Installer - Kitty Functions

_kitty_gui_available() {
  # kitty must exist AND a GUI session must be available
  command -v kitty >/dev/null 2>&1 || return 1
  [[ -n "${WAYLAND_DISPLAY:-}" || -n "${DISPLAY:-}" ]] || return 1
  return 0
}

_kitty_write_conf_block() {
  local kitty_conf="$1"
  local tmp
  tmp="$(mktemp "${HOME}/.kitty.conf.sentinel.XXXXXX")"

  # Preserve existing file, but replace our managed block if present.
  if [[ -f "$kitty_conf" ]]; then
    awk '
      BEGIN {inblk=0}
      /^# SENTINEL KITTY BEGIN$/ {inblk=1; next}
      /^# SENTINEL KITTY END$/ {inblk=0; next}
      inblk==0 {print}
    ' "$kitty_conf" > "$tmp"
  fi

  {
    echo ""
    echo "# SENTINEL KITTY BEGIN"
    echo "# Managed by SENTINEL installer - Kitty Primary CLI Pathway"
    echo "# Optimized settings for SENTINEL module system and TUI responsiveness"
    echo ""
    echo "# Performance and responsiveness"
    echo "update_check_interval 0"
    echo "enable_audio_bell no"
    echo "sync_to_monitor no"
    echo "repaint_delay 5"
    echo "input_delay 1"
    echo "confirm_os_window_close 0"
    echo "allow_remote_control yes"
    echo ""
    echo "# Font and rendering"
    echo "font_family      JetBrains Mono"
    echo "bold_font        auto"
    echo "italic_font      auto"
    echo "bold_italic_font auto"
    echo "font_size 11.0"
    echo ""
    echo "# Colors optimized for SENTINEL"
    echo "foreground #e0e0e0"
    echo "background #0d1117"
    echo "selection_foreground #000000"
    echo "selection_background #30a14e"
    echo ""
    echo "# Cursor"
    echo "cursor #30a14e"
    echo "cursor_text_color #0d1117"
    echo "cursor_shape block"
    echo "cursor_blink_interval 0.5"
    echo ""
    echo "# Window layout"
    echo "window_padding_width 4"
    echo "window_margin_width 0"
    echo "single_window_margin_width -1000"
    echo ""
    echo "# Tab bar"
    echo "tab_bar_edge bottom"
    echo "tab_bar_style powerline"
    echo "tab_powerline_style slanted"
    echo ""
    echo "# Shell integration"
    echo "shell_integration enabled"
    echo ""
    echo "# SENTINEL-specific: Enable kitty as primary CLI"
    echo "shell ${SHELL:-/bin/bash}"
    echo ""
    echo "# SENTINEL startup script (kitty shell integration handles this)"
    echo "shell_integration enabled"
    echo ""
    echo "# SENTINEL KITTY END"
  } >> "$tmp"

  install -m 600 "$tmp" "$kitty_conf"
  rm -f "$tmp" 2>/dev/null || true
}

_kitty_install_sentinel_launcher() {
  local launcher="${HOME}/.local/bin/sentinel-kitty"
  local tmp
  tmp="$(mktemp "${HOME}/.sentinel-kitty.XXXXXX")"

  cat > "$tmp" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

SENTINEL_ROOT="${SENTINEL_ROOT:-$HOME/.sentinel}"

python_bin="${SENTINEL_PYTHON:-}"
if [[ -z "${python_bin}" || ! -x "${python_bin}" ]]; then
  if [[ -x "${HOME}/venv/bin/python3" ]]; then
    python_bin="${HOME}/venv/bin/python3"
  else
    python_bin="$(command -v python3 || true)"
  fi
fi

entry="${SENTINEL_ROOT}/sentinel_toggles_tui.py"
if [[ ! -f "${entry}" ]]; then
  echo "SENTINEL entrypoint not found: ${entry}" >&2
  echo "Set SENTINEL_ROOT or install SENTINEL to use sentinel-kitty." >&2
  exit 127
fi

if [[ -z "${python_bin}" || ! -x "${python_bin}" ]]; then
  echo "python3 not found; cannot run ${entry}" >&2
  exit 127
fi

cmd=( "${python_bin}" "${entry}" "$@" )

# Always run in kitty for primary CLI pathway
if [[ -n "${KITTY_WINDOW_ID:-}" ]]; then
  exec "${cmd[@]}"
fi

if command -v kitty >/dev/null 2>&1 && [[ -n "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ]]; then
  # Run in kitty; use safe quoting for bash -lc
  quoted="$(printf '%q ' "${cmd[@]}")"
  exec kitty -e bash -lc "${quoted}"
fi

exec "${cmd[@]}"
EOF

  install -m 755 "$tmp" "$launcher"
  rm -f "$tmp" 2>/dev/null || true
}

setup_kitty_primary_cli() {
  # Primary CLI pathway - kitty is required
  if is_done "KITTY_PRIMARY_CLI_DONE"; then
    return 0
  fi

  # Check if kitty is available
  if ! _kitty_gui_available; then
    fail "Kitty primary CLI pathway requires kitty to be installed and a GUI session to be available."
    fail "Install kitty: https://sw.kovidgoyal.net/kitty/"
    fail "Requirement: kitty must be in PATH and DISPLAY or WAYLAND_DISPLAY must be set."
    return 1
  fi

  local xdg_conf="${XDG_CONFIG_HOME:-${HOME}/.config}"
  local kitty_dir="${xdg_conf%/}/kitty"
  local kitty_conf="${kitty_dir}/kitty.conf"

  step "Configuring Kitty as primary CLI for SENTINEL"
  safe_mkdir "$kitty_dir" 700
  _kitty_write_conf_block "$kitty_conf"
  ok "Kitty config updated: $kitty_conf"

  step "Installing sentinel-kitty launcher"
  safe_mkdir "${HOME}/.local/bin" 700
  _kitty_install_sentinel_launcher
  ok "Launcher installed: ${HOME}/.local/bin/sentinel-kitty"

  step "Creating kitty.rc for SENTINEL module loading"
  _kitty_create_rc_file
  ok "kitty.rc created"

  step "Installing kitty startup script"
  _kitty_install_startup_script
  ok "kitty_startup.sh installed"

  mark_done "KITTY_PRIMARY_CLI_DONE"
  ok "Kitty primary CLI pathway configured"
}

_kitty_install_startup_script() {
  local startup_script="${HOME}/kitty_startup.sh"
  local tmp
  tmp="$(mktemp "${HOME}/.kitty_startup.XXXXXX")"

  cat > "$tmp" <<'EOF'
#!/usr/bin/env bash
# SENTINEL Kitty Startup Script
# This script is executed when kitty starts a new shell session
# It ensures SENTINEL modules are loaded correctly in kitty

# Mark that we're starting in kitty
export SENTINEL_KITTY_PRIMARY_CLI=1
export SENTINEL_TERMINAL="kitty"

# Source kitty.rc if it exists
if [[ -f "${HOME}/kitty.rc" ]]; then
    source "${HOME}/kitty.rc"
fi

# Ensure kitty integration module is loaded early
if [[ -f "${HOME}/bash_modules.d/kitty_integration.module" ]]; then
    source "${HOME}/bash_modules.d/kitty_integration.module"
elif [[ -f "${SENTINEL_ROOT}/bash_modules.d/kitty_integration.module" ]]; then
    source "${SENTINEL_ROOT}/bash_modules.d/kitty_integration.module"
fi

# Set kitty window title
if [[ -n "${KITTY_WINDOW_ID:-}" ]] && type sentinel_kitty_set_title &>/dev/null; then
    sentinel_kitty_set_title "SENTINEL - $(basename "${PWD}")"
fi
EOF

  install -m 755 "$tmp" "$startup_script"
  rm -f "$tmp" 2>/dev/null || true
}

_kitty_create_rc_file() {
  local kitty_rc="${HOME}/kitty.rc"
  local tmp
  tmp="$(mktemp "${HOME}/.kitty.rc.sentinel.XXXXXX")"

  cat > "$tmp" <<'KITTYRCEOF'
#!/usr/bin/env bash
# kitty.rc - SENTINEL Kitty Primary CLI Configuration
#
# This file is sourced by kitty on startup when using SENTINEL's kitty pathway.
# It loads all SENTINEL modules optimized for kitty's GPU-accelerated terminal.

export LANG=en_US.utf8
export LC_ALL=en_US.utf8

# Mark that we're in kitty primary CLI mode
export SENTINEL_KITTY_PRIMARY_CLI=1
export SENTINEL_TERMINAL="kitty"

# Determine SENTINEL_ROOT dynamically if not already set
if [[ -z "${SENTINEL_ROOT}" ]]; then
    export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

#========================================================================
# Module System Behavior - Global Settings
#----------------------------------------
# These settings control how the module system operates

# Module Security Configuration
# ----------------------------
# These settings control module security verification

# Configuration Caching System
# ------------------------
# Controls how configuration files are cached for faster shell startup
export SENTINEL_CONFIG_CACHE_ENABLED=1
export SENTINEL_CONFIG_FORCE_REFRESH=0
export SENTINEL_CONFIG_CACHE_RETENTION_DAYS=30
export SENTINEL_CONFIG_VERIFY_HASH=1

# Module System Optimization
# -------------------------
# Controls module dependency resolution and loading behavior
export SENTINEL_MODULE_DEBUG=0
export SENTINEL_MODULE_AUTOLOAD=1
export SENTINEL_MODULE_CACHE_ENABLED=1
export SENTINEL_MODULE_VERIFY=1

# Kitty-specific optimizations
export SENTINEL_KITTY_GPU_ACCEL=1
export SENTINEL_TERMINAL_COLORS=256

#========================================================================
# MODULE-SPECIFIC CONFIGURATIONS
#========================================================================

# SENTINEL Feature Module Toggles
# -------------------------------
# Uncomment to enable/disable specific SENTINEL feature modules:
export SENTINEL_OBFUSCATE_ENABLED=1
export SENTINEL_FZF_ENABLED=1
export SENTINEL_ML_ENABLED=1
export SENTINEL_OSINT_ENABLED=1
export SENTINEL_CYBERSEC_ENABLED=1
export SENTINEL_GITSTAR_ENABLED=1
export SENTINEL_CHAT_ENABLED=1
export SENTINEL_SMALLLLM_ENABLED=1
export SENTINEL_AUTO_CASE_CORRECTION_ENABLED=1

# Obfuscation Module
# ------------------
export OBFUSCATE_OUTPUT_DIR="${HOME}/secure/obfuscated_files"

# Hashcat Configuration
# --------------------
export HASHCAT_BIN="/usr/bin/hashcat"
export HASHCAT_WORDLISTS_DIR="/usr/share/wordlists"
export HASHCAT_OUTPUT_DIR="${HOME}/.hashcat/cracked"

# Distcc Configuration
# -------------------
export DISTCC_HOSTS="localhost"
export CCACHE_SIZE="5G"

#========================================================================
# USER CUSTOMIZATIONS
#========================================================================
# Add your custom configurations, aliases, and functions below

# Source centralized configuration

# Load kitty integration module early
if [[ -f "${HOME}/bash_modules.d/kitty_integration.module" ]]; then
    source "${HOME}/bash_modules.d/kitty_integration.module"
fi

# Source module manager and parallel loader
if [[ -f "${HOME}/bash_modules.d/module_manager.module" ]]; then
    source "${HOME}/bash_modules.d/module_manager.module"
fi
if [[ -f "${HOME}/bash_modules.d/parallel_loader.module" ]]; then
    source "${HOME}/bash_modules.d/parallel_loader.module"
fi

# Load all enabled modules in parallel
if type parallel_load_modules &>/dev/null; then
    parallel_load_modules "${HOME}/.bash_modules"
else
    # Fallback: load modules sequentially if parallel loader not available
    if [[ -f "${HOME}/.bash_modules" ]]; then
        while IFS= read -r module_name || [[ -n "$module_name" ]]; do
            [[ -z "$module_name" || "$module_name" == \#* ]] && continue
            if [[ -f "${HOME}/bash_modules.d/${module_name}.module" ]]; then
                source "${HOME}/bash_modules.d/${module_name}.module" 2>/dev/null || true
            fi
        done < "${HOME}/.bash_modules"
    fi
fi

export PATH="$HOME/bin:$PATH"

# Load plugins if available
if [[ -d "${HOME}/bash_modules.d/plugins" ]]; then
    for plugin in "${HOME}/bash_modules.d/plugins"/*.plugin; do
        [[ -f "$plugin" ]] && source "$plugin" 2>/dev/null || true
    done
fi

# ========================================================================
# Performance Configuration
# ========================================================================
export U_LAZY_LOAD=1

# Lazy loading for development tools
if [[ -n "${CONFIG[LAZY_LOAD]+set}" && "${CONFIG[LAZY_LOAD]}" == "1" ]] || [[ "${U_LAZY_LOAD:-1}" == "1" ]]; then
    function pyenv() {
        { unset -f pyenv; } 2>/dev/null || true
        if [[ -d "$HOME/.pyenv" ]]; then
            { export PYENV_ROOT="$HOME/.pyenv"; } 2>/dev/null || true
            { export PATH="$PYENV_ROOT/bin:$PATH"; } 2>/dev/null || true
            { eval "$(command pyenv init - 2>/dev/null)"; } 2>/dev/null || true
            { eval "$(command pyenv virtualenv-init - 2>/dev/null)"; } 2>/dev/null || true
            { pyenv "$@"; } 2>/dev/null || return 0
        else
            { echo "pyenv is not installed"; } 2>/dev/null || true
            return 0
        fi
    }
    function nvm() {
        { unset -f nvm; } 2>/dev/null || true
        if [[ -d "$HOME/.nvm" ]]; then
            { export NVM_DIR="$HOME/.nvm"; } 2>/dev/null || true
            { [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh"; } 2>/dev/null || true
            { [ -s "$NVM_DIR/bash_completion" ] && . "$NVM_DIR/bash_completion"; } 2>/dev/null || true
            { nvm "$@"; } 2>/dev/null || return 0
        else
            { echo "nvm is not installed"; } 2>/dev/null || true
            return 0
        fi
    }
    function node() {
        { unset -f node; } 2>/dev/null || true
        { nvm >/dev/null 2>&1; } 2>/dev/null || true
        { node "$@"; } 2>/dev/null || return 0
    }
    function npm() {
        { unset -f npm; } 2>/dev/null || true
        { nvm >/dev/null 2>&1; } 2>/dev/null || true
        { npm "$@"; } 2>/dev/null || return 0
    }
    function python() {
        { unset -f python; } 2>/dev/null || true
        { pyenv >/dev/null 2>&1; } 2>/dev/null || true
        { python "$@"; } 2>/dev/null || return 0
    }
    function pip() {
        { unset -f pip; } 2>/dev/null || true
        { pyenv >/dev/null 2>&1; } 2>/dev/null || true
        { pip "$@"; } 2>/dev/null || return 0
    }
fi

# Silence module status messages
export SENTINEL_QUIET_STATUS=1

# =============================
# End of kitty.rc
# =============================
KITTYRCEOF

  install -m 644 "$tmp" "$kitty_rc"
  rm -f "$tmp" 2>/dev/null || true
}

patch_bashrc_for_kitty() {
  local rc="$1"
  local sentinel_bashrc="${PROJECT_ROOT}/bashrc"

  # Check if .bashrc is owned by root or not writable
  if [[ -e "$rc" && ! -w "$rc" ]]; then
    warn "Cannot write to $rc (permission denied, may be owned by root)"
    step "Creating a new bashrc file for kitty pathway"

    local user_bashrc="${HOME}/.bashrc.sentinel-kitty"

    if [[ -f "$sentinel_bashrc" ]]; then
      safe_cp "$sentinel_bashrc" "$user_bashrc"
      chmod 644 "$user_bashrc"
      ok "SENTINEL bashrc installed as $user_bashrc"

      if [[ $INTERACTIVE -eq 1 ]]; then
        read -r -t 30 -p "Would you like to add a line to source $user_bashrc from your $rc? You may need to enter sudo password. [y/N]: " confirm || confirm="n"
      else
        confirm="n"
        log "Non-interactive mode: using default answer '$confirm'"
      fi

      if [[ "$confirm" =~ ^[Yy]([Ee][Ss])?$ ]]; then
        sudo bash -c "echo '' >> $rc"
        sudo bash -c "echo '# SENTINEL Framework Integration - Kitty Primary CLI' >> $rc"
        sudo bash -c "echo \"if [[ -f \\\"${user_bashrc}\\\" ]]; then\" >> $rc"
        sudo bash -c "echo \"    source \\\"${user_bashrc}\\\"\" >> $rc"
        sudo bash -c "echo 'fi' >> $rc"
        ok "Added sourcing line to $rc via sudo"
      else
        echo "Please manually add the following lines to your $rc:"
        echo ""
        echo "# SENTINEL Framework Integration - Kitty Primary CLI"
        echo "if [[ -f \"${user_bashrc}\" ]]; then"
        echo "    source \"${user_bashrc}\""
        echo "fi"
        ok "Created $user_bashrc but you'll need to source it manually"
      fi

      # Add line to source kitty.rc from the user_bashrc
      if ! grep -q "source.*kitty.rc" "$user_bashrc"; then
        {
          echo ''
          echo '# SENTINEL Kitty Primary CLI'
          echo "if [[ -f \"\${HOME}/kitty.rc\" ]]; then"
          echo "    source \"\${HOME}/kitty.rc\""
          echo 'fi'
        } >> "$user_bashrc"
        ok "Added line to source kitty.rc in $user_bashrc"
      fi

      return 0
    else
      fail "SENTINEL bashrc not found at $sentinel_bashrc"
      return 1
    fi
  fi

  # Normal flow for writable .bashrc
  if [[ -f "$rc" ]]; then
    safe_cp "$rc" "$rc.sentinel-kitty.bak.$(date +%s)"
    ok "Backed up $rc to $rc.sentinel-kitty.bak.$(date +%s)"
  fi

  step "Patching $rc for SENTINEL Kitty Primary CLI"
  if ! grep -q "source.*kitty.rc" "$rc"; then
    {
      echo ''
      echo '# SENTINEL Framework Integration - Kitty Primary CLI'
      echo "export SENTINEL_ROOT=\"${PROJECT_ROOT}\""
      echo "export SENTINEL_KITTY_PRIMARY_CLI=1"
      echo "if [[ -f \"\${HOME}/kitty.rc\" ]]; then"
      echo "    source \"\${HOME}/kitty.rc\""
      echo 'fi'
    } >> "$rc"
    ok "Patched $rc to load SENTINEL Kitty Primary CLI"
  else
    ok "SENTINEL Kitty Primary CLI already integrated in $rc"
  fi
}
