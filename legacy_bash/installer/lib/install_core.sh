#!/usr/bin/env bash
# SENTINEL Installer - Core Installation

# Prompt for custom user environment
prompt_custom_env

# Setup directories
setup_directories

# Setup Wave Terminal only if not in headless mode
if [[ "${SENTINEL_SKIP_WAVE:-0}" != "1" ]]; then
  setup_wave_terminal
else
  log "Skipping Wave Terminal configuration (headless mode)"
fi

# Install BLE.sh only if not in headless mode
if [[ "${SENTINEL_SKIP_BLESH:-0}" != "1" ]]; then
  if ! is_done "BLESH_INSTALLED"; then
    if [[ -f "${BLESH_DIR}/ble.sh" ]]; then
      ok "BLE.sh already present – skipping clone"
    else
      install_blesh
    fi
    mark_done "BLESH_INSTALLED"
  fi
  create_blesh_loader
else
  log "Skipping BLE.sh installation (headless mode - not needed for server environments)"
  # Mark as done to prevent future attempts but still ensure the loader stub exists
  is_done "BLESH_INSTALLED" || mark_done "BLESH_INSTALLED"
  is_done "BLESH_SKIPPED" || mark_done "BLESH_SKIPPED"
  create_blesh_loader
fi

# Optional Kitty GPU-accelerated terminal integration (never required)
setup_kitty_integration

# Setup bash
if ! is_done "BASHRC_PATCHED"; then
  patch_bashrc "${HOME}/.bashrc"
  mark_done "BASHRC_PATCHED"
fi
copy_postcustom_bootstrap
ensure_local_bin_in_path
copy_bash_modules
copy_shell_support_files
enable_fzf_module
secure_permissions
