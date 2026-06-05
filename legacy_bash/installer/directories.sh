#!/usr/bin/env bash
# SENTINEL Installer - Directory Setup Functions

setup_directories() {
    if is_done "DIRS_CREATED"; then
        if [[ -d "${HOME}/logs" && -d "${HOME}/bash_modules.d" ]]; then
            ok "Directory tree already exists"
            return
        else
            warn "State file marked DIRS_CREATED but directories missing, re-creating"
        fi
    fi
    step "Creating directory tree under ${HOME}"

    # Create directories with error checking
    local dirs=(
        "${HOME}/autocomplete/snippets"
        "${HOME}/autocomplete/context"
        "${HOME}/autocomplete/projects"
        "${HOME}/autocomplete/params"
        "${HOME}/logs"
        "${HOME}/bash_modules.d"
        "${HOME}/.cache/blesh"
        "${HOME}/bash_aliases.d"
        "${HOME}/bash_completion.d"
        "${HOME}/bash_functions.d"
        "${HOME}/contrib"
        "${HOME}/.local/bin"  # Added for pip installations
    )

    for dir in "${dirs[@]}"; do
        safe_mkdir "$dir"
    done

    # Set secure permissions
    chmod 700 "${LOG_DIR}" \
        "${HOME}/"{bash_aliases.d,bash_completion.d,bash_functions.d,contrib} || \
        fail "Failed to set directory permissions"

    mark_done "DIRS_CREATED"
    ok "Directory tree ready"
}

setup_wave_terminal() {
    if is_done "WAVE_TERMINAL_CONFIGURED"; then
        ok "Wave terminal already configured"
        return
    fi
    step "Configuring Wave terminal"

    local wave_config_dir="${HOME}/.config/wave"
    safe_mkdir "$wave_config_dir"

    local wave_shell_init_sh="${wave_config_dir}/shell-init.sh"
    if [[ ! -f "$wave_shell_init_sh" ]]; then
        step "Creating Wave terminal shell-init.sh"
        install -m 644 /dev/null "$wave_shell_init_sh"
        cat > "$wave_shell_init_sh" <<'EOF'
# Source .bashrc to get all bash configs
[ -f ~/.bashrc ] && source ~/.bashrc
EOF
        ok "Wave terminal shell-init.sh created"
    else
        ok "Wave terminal shell-init.sh already exists"
    fi

    local wave_config_yaml="${wave_config_dir}/config.yaml"
    if [[ ! -f "$wave_config_yaml" ]]; then
        step "Creating Wave terminal config.yaml"
        install -m 644 /dev/null "$wave_config_yaml"
        cat > "$wave_config_yaml" <<'EOF'
# ~/.config/wave/config.yaml
# Main Wave Terminal Configuration

# Terminal settings
terminal:
  font_family: "JetBrains Mono"
  font_size: 14
  cursor_style: "block"  # block, underline, bar
  cursor_blink: true
  scrollback: 10000

# Shell configuration
shell:
  program: "/bin/bash"  # or /bin/zsh, /usr/bin/fish
  args: ["--login", "-i"]

  # Custom environment variables
  env:
    TERM: "xterm-256color"
    WAVE_TERMINAL: "1"

  # Shell initialization script
  init_script: "~/.config/wave/shell-init.sh"

# Theme settings
theme:
  name: "dark"  # or "light", "custom"
  # For custom themes, create ~/.config/wave/themes/custom.json

# Window settings
window:
  opacity: 0.95
  blur: true
  decorations: true

# Keybindings (or use keybindings.json)
keybindings:
  - { key: "ctrl+shift+t", action: "new_tab" }
  - { key: "ctrl+shift+w", action: "close_tab" }
  - { key: "ctrl+shift+c", action: "copy" }
  - { key: "ctrl+shift+v", action: "paste" }

# Features
features:
  gpu_acceleration: true
  ligatures: true
  semantic_zoom: true
EOF
        ok "Wave terminal config.yaml created"
    else
        ok "Wave terminal config.yaml already exists"
    fi

    mark_done "WAVE_TERMINAL_CONFIGURED"
    ok "Wave terminal configured"
}
