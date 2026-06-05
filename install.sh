#!/usr/bin/env bash
###############################################################################
# SENTINEL - WezTerm Edition Installer
# -----------------------------------------------
# This script symlinks the WezTerm Lua architecture and links the Thin Bash
# modules into your environment.
###############################################################################

set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

echo "=> Bootstrapping SENTINEL WezTerm Edition..."

# 1. Ensure WezTerm is installed
if ! command -v wezterm &> /dev/null; then
    echo "[!] WezTerm is not installed! Please install WezTerm first."
    exit 1
fi

# 2. Setup WezTerm Configuration
echo "=> Symlinking WezTerm Lua architecture..."
mkdir -p ~/.config/wezterm
# Symlink the sentinel lua modules
if [[ ! -L ~/.config/wezterm/sentinel ]]; then
    ln -sfn "${PROJECT_ROOT}/wezterm_config/sentinel" ~/.config/wezterm/sentinel
fi

# Link main wezterm.lua if it doesn't exist
if [[ ! -f ~/.wezterm.lua && ! -L ~/.wezterm.lua ]]; then
    ln -sfn "${PROJECT_ROOT}/wezterm_config/wezterm.lua" ~/.wezterm.lua
fi

# 3. Setup the Thin Bash Directory
echo "=> Setting up Thin Bash structure..."
mkdir -p ~/.config/sentinel
if [[ ! -L ~/.config/sentinel/thin_bashrc ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/thin_bashrc" ~/.config/sentinel/thin_bashrc
fi
# Ensure directories exist
mkdir -p ~/.config/sentinel/{bash_aliases.d,bash_functions.d,completions.d}

# 4. Patch .bashrc to source thin_bashrc if not already there
if ! grep -q "sentinel/thin_bashrc" ~/.bashrc; then
    echo "=> Patching ~/.bashrc..."
    echo -e "\n# SENTINEL Thin Bash" >> ~/.bashrc
    echo "[[ -f ~/.config/sentinel/thin_bashrc ]] && source ~/.config/sentinel/thin_bashrc" >> ~/.bashrc
fi

# 5. Trigger WezTerm validation
echo "=> Validating WezTerm configuration..."
wezterm show-keys &> /dev/null || echo "[!] Validation skipped or failed, but installation finished."

echo "=> Bootstrap complete! SENTINEL is now powered by WezTerm natively."
echo "=> Please restart your terminal or launch 'wezterm' to begin."
