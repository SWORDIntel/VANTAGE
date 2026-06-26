#!/usr/bin/env bash
# VANTAGE Installer - Bash Functions

ensure_local_bin_in_path() {
    step "Ensuring ~/.local/bin is in PATH"
    local postcustom="${HOME}/bashrc.postcustom"

    if [[ ! -f "$postcustom" ]]; then
        touch "$postcustom"
        chmod 644 "$postcustom"
    fi

    if ! grep -q '\.local/bin' "$postcustom"; then
        {
            echo ''
            echo '# Ensure ~/.local/bin is in PATH for pip-installed tools'
            # shellcheck disable=SC2016
            echo 'if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then'
            # shellcheck disable=SC2016
            echo '    export PATH="$HOME/.local/bin:$PATH"'
            echo 'fi'
        } >> "$postcustom"
        ok "Added ~/.local/bin to PATH in bashrc.postcustom"
    else
        ok "${HOME}/.local/bin already in PATH configuration"
    fi
}

patch_bashrc() {
  local rc="$1"
  local vantage_bashrc="${PROJECT_ROOT}/bashrc"

  # Check if .bashrc is owned by root or not writable
  if [[ -e "$rc" && ! -w "$rc" ]]; then
    warn "Cannot write to $rc (permission denied, may be owned by root)"
    step "Creating a new bashrc file and requesting to source it from your existing .bashrc"

    # Create a separate user bashrc file
    local user_bashrc="${HOME}/.bashrc.vantage"

    # Copy VANTAGE bashrc to user_bashrc if available
    if [[ -f "$vantage_bashrc" ]]; then
      safe_cp "$vantage_bashrc" "$user_bashrc"
      chmod 644 "$user_bashrc"
      ok "VANTAGE bashrc installed as $user_bashrc"

      # Prompt to add sourcing line to original .bashrc via sudo
      if [[ $INTERACTIVE -eq 1 ]]; then
        read -r -t 30 -p "Would you like to add a line to source $user_bashrc from your $rc? You may need to enter sudo password. [y/N]: " confirm || confirm="n"
      else
        confirm="n"
        log "Non-interactive mode: using default answer '$confirm'"
      fi

      if [[ "$confirm" =~ ^[Yy]([Ee][Ss])?$ ]]; then
        # Use sudo to modify the root-owned .bashrc
        sudo bash -c "echo '' >> $rc"
        sudo bash -c "echo '# VANTAGE Framework Integration' >> $rc"
        sudo bash -c "echo \"if [[ -f \\\"${user_bashrc}\\\" ]]; then\" >> $rc"
        sudo bash -c "echo \"    source \\\"${user_bashrc}\\\"\" >> $rc"
        sudo bash -c "echo 'fi' >> $rc"
        ok "Added sourcing line to $rc via sudo"
      else
        echo "Please manually add the following lines to your $rc:"
        echo ""
        echo "# VANTAGE Framework Integration"
        echo "if [[ -f \"${user_bashrc}\" ]]; then"
        echo "    source \"${user_bashrc}\""
        echo "fi"
        ok "Created $user_bashrc but you'll need to source it manually"
      fi

      # Add line to source bashrc.postcustom from the user_bashrc
      if ! grep -q "source.*bashrc.postcustom" "$user_bashrc"; then
        {
          echo ''
          echo '# VANTAGE Extensions'
          echo "if [[ -f \"\${HOME}/bashrc.postcustom\" ]]; then"
          echo "    source \"\${HOME}/bashrc.postcustom\""
          echo 'fi'
        } >> "$user_bashrc"
        ok "Added line to source bashrc.postcustom in $user_bashrc"
      fi

      return 0
    else
      fail "VANTAGE bashrc not found at $vantage_bashrc"
      return 1
    fi
  fi

  # Normal flow for writable .bashrc
  if [[ -f "$rc" ]]; then
    safe_cp "$rc" "$rc.vantage.bak.$(date +%s)"
    ok "Backed up $rc to $rc.vantage.bak.$(date +%s)"
  fi
  step "Prompting for full replacement of $rc with VANTAGE bashrc"
  if [[ $INTERACTIVE -eq 1 ]]; then
    read -r -t 30 -p "Replace your $rc with VANTAGE's secure version? [y/N]: " confirm || confirm="n"
  else
    confirm="n"
    log "Non-interactive mode: using default answer '$confirm'"
  fi
  if [[ "$confirm" =~ ^[Yy]([Ee][Ss])?$ ]]; then
    if [[ -f "$vantage_bashrc" ]]; then
      safe_cp "$vantage_bashrc" "$rc"
      {
        echo ''
        echo '# VANTAGE Framework Root'
        echo "export VANTAGE_ROOT=\"${PROJECT_ROOT}\""
      } >> "$rc"
      chmod 644 "$rc"
      ok "VANTAGE bashrc installed as $rc"
      log "Replaced $rc with VANTAGE bashrc at $(date)"
    else
      warn "VANTAGE bashrc not found at $vantage_bashrc; skipping replacement."
    fi
  else
    step "Patching existing bashrc to load VANTAGE"
    if ! grep -q "source.*waveterm.rc" "$rc"; then
      {
        echo ''
        echo '# VANTAGE Framework Integration'
        echo "export VANTAGE_ROOT=\"${PROJECT_ROOT}\""
        echo "if [[ -f \"\${HOME}/waveterm.rc\" ]]; then"
        echo "    # Safe loading mechanism that won't crash the terminal"
        echo "    source \"\${HOME}/waveterm.rc\" 2>/dev/null || echo \"[bashrc] Warning: Failed to load waveterm.rc\" >&2"
        echo 'fi'
      } >> "$rc"
      ok "Patched $rc to load VANTAGE"
    else
      ok "VANTAGE already integrated in $rc"
    fi
  fi
}

copy_postcustom_bootstrap() {
    if ! is_done "POSTCUSTOM_READY"; then
      step "Deploying bashrc.postcustom and waveterm.rc"
      install -m 644 "${PROJECT_ROOT}/bashrc.postcustom" "${HOME}/bashrc.postcustom"
      install -m 644 "${PROJECT_ROOT}/legacy_bash/waveterm.rc" "${HOME}/waveterm.rc"


      ok "bashrc.postcustom and waveterm.rc in place with VENV_AUTO enabled"
      mark_done "POSTCUSTOM_READY"
    fi
}

copy_bash_modules() {
    if ! is_done "CORE_MODULES_INSTALLED"; then
      MODULE_SRC="${PROJECT_ROOT}/bash_modules.d"
      MODULE_HELPER_SRC="${PROJECT_ROOT}/tools/module_helpers"

      if [[ -d "$MODULE_SRC" ]]; then
        step "Copying core bash modules from '${MODULE_SRC}/'"
        mkdir -p "${MODULES_DIR}"
        rsync -a --delete \
          --include='*/' \
          --include='*.module' \
          --include='*.plugin' \
          --include='*.conf' \
          --include='README.md' \
          --include='*.README.md' \
          --exclude='*' \
          "${MODULE_SRC}/" "${MODULES_DIR}/"
        find "${MODULES_DIR}" -type d -exec chmod 700 {} \;
        find "${MODULES_DIR}" -type f -exec chmod 600 {} \;
        ok "Modules synced → ${MODULES_DIR}"

        # Automatically run install-autocomplete.sh if present
        AUTOCOMPLETE_INSTALLER="${MODULE_HELPER_SRC}/install-autocomplete.sh"
        if [[ -f "$AUTOCOMPLETE_INSTALLER" ]]; then
          step "Running modular autocomplete installer"
          # Export MODULES_DIR explicitly for install-autocomplete.sh
          export MODULES_DIR="${MODULES_DIR}"
          bash "$AUTOCOMPLETE_INSTALLER" || warn "install-autocomplete.sh failed; check logs."
          ok "Modular autocomplete installer completed"
        else
          warn "install-autocomplete.sh not found in $MODULE_HELPER_SRC; autocomplete modules may not be fully installed."
        fi
      else
        warn "No bash_modules.d/ directory found – skipping module sync"
      fi

      mark_done "CORE_MODULES_INSTALLED"
    fi
}

copy_shell_support_files() {
    if ! is_done "SHELL_SUPPORT_COPIED"; then
      step "Copying shell support files to HOME"

      # Copy main files
      for file in bash_aliases bash_completion bash_functions; do
        if [[ -f "${PROJECT_ROOT}/$file" ]]; then
          step "Installing $file to $HOME/.$file"
          cp "${PROJECT_ROOT}/$file" "$HOME/.$file"
          chmod 644 "$HOME/.$file"
          ok "$file installed"
        else
          warn "$file not found in repository"
        fi
      done

      # Copy support directories
      for dir in bash_aliases.d bash_completion.d bash_functions.d contrib; do
        SRC_DIR="${PROJECT_ROOT}/$dir"
        DST_DIR="${HOME}/$dir"

        if [[ -d "$SRC_DIR" ]]; then
          step "Copying $dir content to $DST_DIR"
          rsync -a "$SRC_DIR/" "$DST_DIR/"

          # Set appropriate permissions
          find "$DST_DIR" -type d -exec chmod 700 {} \;
          find "$DST_DIR" -type f -exec chmod 600 {} \;

          ok "$dir content copied with secure permissions"
        else
          warn "$SRC_DIR not found; skipping."
        fi
      done

      # Create .bash_modules file if it doesn't exist
      if [[ ! -f "$HOME/.bash_modules" ]]; then
        cp "${PROJECT_ROOT}/.bash_modules" "$HOME/.bash_modules"
        chmod 644 "$HOME/.bash_modules"
        ok "Created $HOME/.bash_modules"
      fi

      mark_done "SHELL_SUPPORT_COPIED"
    fi
}

enable_fzf_module() {
    FZF_BIN="$(command -v fzf 2>/dev/null || true)"
    POSTCUSTOM_FILE="${HOME}/bashrc.postcustom"

    if [[ -n "$FZF_BIN" ]]; then
      step "fzf detected at $FZF_BIN; enabling VANTAGE FZF module"
      if ! grep -q '^export VANTAGE_FZF_ENABLED=1' "$POSTCUSTOM_FILE"; then
        echo 'export VANTAGE_FZF_ENABLED=1  # Enable FZF integration' >> "$POSTCUSTOM_FILE"
        ok "Enabled VANTAGE FZF module in $POSTCUSTOM_FILE"
      else
        ok "VANTAGE FZF module already enabled"
      fi
    else
      warn "fzf not found; VANTAGE FZF module not enabled. Install fzf and set export VANTAGE_FZF_ENABLED=1 in $POSTCUSTOM_FILE to enable."
    fi
}

_vantage_kitty_gui_available() {
  # kitty must exist AND a GUI session must be available
  command -v kitty >/dev/null 2>&1 || return 1
  [[ -n "${WAYLAND_DISPLAY:-}" || -n "${DISPLAY:-}" ]] || return 1
  return 0
}

_vantage_write_kitty_conf_block() {
  local kitty_conf="$1"
  local tmp
  tmp="$(mktemp "${HOME}/.kitty.conf.vantage.XXXXXX")"

  # Preserve existing file, but replace our managed block if present.
  if [[ -f "$kitty_conf" ]]; then
    awk '
      BEGIN {inblk=0}
      /^# VANTAGE KITTY BEGIN$/ {inblk=1; next}
      /^# VANTAGE KITTY END$/ {inblk=0; next}
      inblk==0 {print}
    ' "$kitty_conf" > "$tmp"
  fi

  {
    echo ""
    echo "# VANTAGE KITTY BEGIN"
    echo "# Managed by VANTAGE installer. Safe to delete to disable."
    echo "# Conservative settings focused on TUI responsiveness."
    echo "update_check_interval 0"
    echo "enable_audio_bell no"
    echo "sync_to_monitor no"
    echo "repaint_delay 5"
    echo "input_delay 1"
    echo "confirm_os_window_close 0"
    echo "allow_remote_control no"
    echo "# VANTAGE KITTY END"
  } >> "$tmp"

  install -m 600 "$tmp" "$kitty_conf"
  rm -f "$tmp" 2>/dev/null || true
}

_vantage_install_vantage_tty_launcher() {
  local launcher="${HOME}/.local/bin/vantage-tty"
  local tmp
  tmp="$(mktemp "${HOME}/.vantage-tty.XXXXXX")"

  cat > "$tmp" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

VANTAGE_ROOT="${VANTAGE_ROOT:-$HOME/.vantage}"

python_bin="${VANTAGE_PYTHON:-}"
if [[ -z "${python_bin}" || ! -x "${python_bin}" ]]; then
  if [[ -x "${HOME}/venv/bin/python3" ]]; then
    python_bin="${HOME}/venv/bin/python3"
  else
    python_bin="$(command -v python3 || true)"
  fi
fi

entry="${VANTAGE_ROOT}/contrib/vantage_toggles_tui.py"
if [[ ! -f "${entry}" ]]; then
  echo "VANTAGE entrypoint not found: ${entry}" >&2
  echo "Set VANTAGE_ROOT or install VANTAGE to use vantage-tty." >&2
  exit 127
fi

if [[ -z "${python_bin}" || ! -x "${python_bin}" ]]; then
  echo "python3 not found; cannot run ${entry}" >&2
  exit 127
fi

cmd=( "${python_bin}" "${entry}" "$@" )

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

setup_kitty_integration() {
  # Optional feature; never required; safe to rerun
  if is_done "KITTY_INTEGRATION_DONE"; then
    return 0
  fi

  # Skip automatically in headless / non-interactive installs
  if [[ "${VANTAGE_HEADLESS:-0}" == "1" ]]; then
    log "Skipping Kitty integration (headless mode)"
    mark_done "KITTY_INTEGRATION_DONE"
    return 0
  fi
  if [[ "${INTERACTIVE:-1}" -eq 0 ]]; then
    log "Skipping Kitty integration (non-interactive mode)"
    mark_done "KITTY_INTEGRATION_DONE"
    return 0
  fi

  step "Enable Kitty GPU-accelerated terminal integration? (optional)"
  local confirm="n"
  read -r -t 30 -p "Enable Kitty GPU-accelerated terminal integration? [y/N]: " confirm || confirm="n"
  if [[ ! "$confirm" =~ ^[Yy]([Ee][Ss])?$ ]]; then
    log "Kitty integration not enabled"
    mark_done "KITTY_INTEGRATION_DONE"
    return 0
  fi

  if ! _vantage_kitty_gui_available; then
    warn "Kitty integration requested, but kitty/GUI session not detected. Continuing without enabling."
    warn "Requirement: kitty must be in PATH and DISPLAY or WAYLAND_DISPLAY must be set."
    mark_done "KITTY_INTEGRATION_DONE"
    return 0
  fi

  local xdg_conf="${XDG_CONFIG_HOME:-${HOME}/.config}"
  local kitty_dir="${xdg_conf%/}/kitty"
  local kitty_conf="${kitty_dir}/kitty.conf"

  step "Configuring Kitty for low-latency TUI"
  safe_mkdir "$kitty_dir" 700
  _vantage_write_kitty_conf_block "$kitty_conf"
  ok "Kitty config updated: $kitty_conf"

  step "Installing vantage-tty launcher"
  safe_mkdir "${HOME}/.local/bin" 700
  _vantage_install_vantage_tty_launcher
  ok "Launcher installed: ${HOME}/.local/bin/vantage-tty"

  mark_done "KITTY_INTEGRATION_DONE"
  ok "Kitty integration enabled (optional)"
}

secure_permissions() {
    if ! is_done "PERMISSIONS_SECURED"; then
      step "Securing permissions on all VANTAGE files and modules"

      # Secure all directories
      find "${HOME}/bash_modules.d" -type d -exec chmod 700 {} \;
      find "${HOME}/logs" -type d -exec chmod 700 {} \;

      # Secure all files
      find "${HOME}/bash_modules.d" -type f -exec chmod 600 {} \;

      # Make executable files executable
      find "${HOME}/bash_aliases.d" -type f -exec chmod 700 {} \;
      find "${HOME}/.local/bin" -type f -exec chmod 755 {} \; 2>/dev/null || true

      # Secure .bashrc in home if it's writable
      if [[ -f "$HOME/.bashrc" && -w "$HOME/.bashrc" ]]; then
        chmod 644 "$HOME/.bashrc"
        ok "Secured permissions on $HOME/.bashrc"
      elif [[ -f "$HOME/.bashrc" ]]; then
        warn "Cannot change permissions on $HOME/.bashrc (not writable)"
      fi

      # Secure .bashrc.vantage if it exists instead
      if [[ -f "$HOME/.bashrc.vantage" ]]; then
        chmod 644 "$HOME/.bashrc.vantage"
        ok "Secured permissions on $HOME/.bashrc.vantage"
      fi

      # Secure .blerc if present
      if [[ -f "$HOME/.blerc" && -w "$HOME/.blerc" ]]; then
        chmod 600 "$HOME/.blerc"
        ok "Secured permissions on $HOME/.blerc"
      elif [[ -f "$HOME/.blerc" ]]; then
        warn "Cannot change permissions on $HOME/.blerc (not writable)"
      fi

      # Secure cache directory
      if [[ -d "$HOME/.cache/blesh" ]]; then
        chmod 700 "$HOME/.cache/blesh"
        ok "Secured permissions on $HOME/.cache/blesh"
      fi

      ok "Secure permissions set on all files and directories"
      mark_done "PERMISSIONS_SECURED"
    fi
}

run_verification_checks() {
    step "Verifying installation"

    # Ensure autocomplete directory exists before verification
    if [[ ! -d "${HOME}/autocomplete" ]]; then
      step "Creating missing autocomplete directory"
      safe_mkdir "${HOME}/autocomplete" 755
      safe_mkdir "${HOME}/autocomplete/snippets" 755
      safe_mkdir "${HOME}/autocomplete/context" 755
      safe_mkdir "${HOME}/autocomplete/projects" 755
      safe_mkdir "${HOME}/autocomplete/params" 755

      # Ensure proper permissions explicitly
      chmod 755 "${HOME}/autocomplete" 2>/dev/null
      find "${HOME}/autocomplete" -type d -exec chmod 755 {} \; 2>/dev/null

      # Make any executable scripts actually executable
      find "${HOME}/autocomplete" -type f -name "*.sh" -exec chmod 755 {} \; 2>/dev/null

      ok "Autocomplete directories created with proper permissions"
    fi

    # Check that essential directories exist
    for dir in "${HOME}/autocomplete" "${MODULES_DIR}" "${HOME}/.local/bin"; do
      if [[ ! -d "$dir" ]]; then
        warn "Essential directory $dir is missing!"
      else
        ok "Directory $dir exists"
      fi
    done

    # Check that essential files exist
    for file in "${HOME}/.bashrc" "${HOME}/bashrc.postcustom" "${BLESH_LOADER}"; do
      if [[ ! -f "$file" ]]; then
        warn "Essential file $file is missing!"
      else
        ok "File $file exists"
      fi
    done

    # Check that Python venv exists and has basic packages
    VENV_PYTHON="${HOME}/venv/bin/python3"
    if [[ "${VANTAGE_SKIP_PYTHON_VENV:-0}" == "1" ]]; then
      log "Skipping Python venv verification (VANTAGE_SKIP_PYTHON_VENV=1)"
    elif [[ ! -f "$VENV_PYTHON" ]]; then
      warn "Python virtual environment not properly installed"
    else
      ok "Python virtual environment found at ${HOME}/venv"

      # Test importing a few key packages
      for pkg in numpy markovify tqdm; do
        if ! "$VENV_PYTHON" -c "import $pkg" &>/dev/null; then
          warn "Python package $pkg not installed in venv"
        else
          ok "Python package $pkg installed correctly"
        fi
      done
    fi

    # Check that autocomplete is installed
    if [[ ! -f "${HOME}/bash_aliases.d/autocomplete" ]]; then
      warn "Autocomplete script not found in ${HOME}/bash_aliases.d/"
    else
      ok "Autocomplete script installed"
    fi

    # Verify PATH configuration
    if grep -q '\.local/bin' "${HOME}/bashrc.postcustom"; then
      ok "PATH configuration includes ~/.local/bin"
    else
      warn "PATH may not include ~/.local/bin"
    fi
}

final_summary() {
    echo
    ok "Installation completed successfully!"
    echo "Installer version: ${INSTALLER_VERSION}"
    echo
    echo "• Open a new terminal OR run:  source '${HOME}/bashrc.postcustom'"
    echo "• Verify with:                @autocomplete status"
    echo "• Logs:                       ${LOG_DIR}/install.log"
    echo "• Rollback if needed:         ${ROLLBACK_SCRIPT}"
    echo

    # Add specific guidance for Debian login shells
    echo "Important for Debian/Ubuntu users:"
    echo "• If using login shells (common with GUI terminals), ensure your ~/.profile or ~/.bash_profile"
    echo "  sources ~/.bashrc so VANTAGE loads correctly. Add these lines if missing:"
    echo "    if [ -f \"$HOME/.bashrc\" ]; then"
    echo "        . \"$HOME/.bashrc\""
    echo "    fi"
    echo

    echo "If you encounter issues after installation:"
    echo "1. Run: @autocomplete fix"
    echo "2. Ensure ~/.profile sources ~/.bashrc (see above)"
    echo "3. Check PATH includes ~/.local/bin: echo \$PATH"
    echo "4. If problems persist, run: bash $0"
    echo

    # Run post-install check if present
    POSTINSTALL_CHECK_SCRIPT="${PROJECT_ROOT}/scripts/vantage_postinstall_check.sh"
    if [[ -f "$POSTINSTALL_CHECK_SCRIPT" ]]; then
      step "Running VANTAGE post-installation verification"
      bash "$POSTINSTALL_CHECK_SCRIPT"
      ok "Post-installation verification complete. See summary above."
    else
      warn "Post-installation verification script not found at $POSTINSTALL_CHECK_SCRIPT. Skipping."
    fi

    # Create a summary file for referencem
    SUMMARY_FILE="${HOME}/VANTAGE_INSTALL_SUMMARY.txt"
    {
      echo "VANTAGE Installation Summary"
      echo "============================="
      echo "Date: $(date)"
      echo "Version: ${INSTALLER_VERSION}"
      echo "Python: $PYTHON_CMD"
      echo "Install State: ${STATE_FILE}"
      echo "Rollback Script: ${ROLLBACK_SCRIPT}"
      echo ""
      echo "Key Locations:"
      echo "- Modules: ${MODULES_DIR}"
      echo "- Logs: ${LOG_DIR}"
      echo "- Python venv: ${HOME}/venv"
      echo "- BLE.sh: ${BLESH_DIR}"
      echo ""
      echo "To activate VANTAGE in new shells, one of these should be present:"
      echo "1. ${HOME}/.bashrc sources ${HOME}/bashrc.postcustom"
      echo "2. ${HOME}/.bashrc.vantage is sourced from ${HOME}/.bashrc"
      echo ""
      echo "For support, check the logs at ${LOG_DIR}/install.log"
    } > "$SUMMARY_FILE"

    ok "Installation summary saved to ${SUMMARY_FILE}"

    # Optional outbound-only telemetry; never blocks installer completion
    installer_fabric_emit_event "installer_complete" "{\"status\":\"success\",\"version\":\"${INSTALLER_VERSION}\"}" || true
}
