#!/usr/bin/env bash
###############################################################################
# SENTINEL - WezTerm Edition Installer
# -----------------------------------------------
# This script symlinks the WezTerm Lua architecture and links the Thin Bash
# modules into your environment.
###############################################################################

set -euo pipefail

# ANSI Colors for Rich Feedback
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

echo -e "${CYAN}======================================================${NC}"
echo -e "${CYAN}          SENTINEL WEZTERM EDITION INSTALLER          ${NC}"
echo -e "${CYAN}======================================================${NC}\n"

# 1. Ensure WezTerm is installed
echo -e "${BLUE}[*] Checking dependencies...${NC}"
if ! command -v wezterm &> /dev/null; then
    echo -e "    ${RED}❌ WezTerm is not installed! Please install WezTerm first.${NC}"
    exit 1
fi
echo -e "    ${GREEN}✔ WezTerm found!${NC}"

# 2. Setup WezTerm Configuration
echo -e "\n${BLUE}[*] Setting up native WezTerm Lua architecture...${NC}"
mkdir -p ~/.config/wezterm/plugins
if [[ ! -L ~/.config/wezterm/plugins/sentinel.wezterm ]]; then
    ln -sfn "${PROJECT_ROOT}" ~/.config/wezterm/plugins/sentinel.wezterm
    echo -e "    ${GREEN}✔ Symlinked ~/.config/wezterm/plugins/sentinel.wezterm${NC}"
else
    echo -e "    ${YELLOW}⚠ ~/.config/wezterm/plugins/sentinel.wezterm already linked.${NC}"
fi

if [[ ! -f ~/.wezterm.lua && ! -L ~/.wezterm.lua ]]; then
    ln -sfn "${PROJECT_ROOT}/wezterm_config/wezterm.lua" ~/.wezterm.lua
    echo -e "    ${GREEN}✔ Symlinked ~/.wezterm.lua entrypoint${NC}"
else
    echo -e "    ${YELLOW}⚠ ~/.wezterm.lua already exists. Skipping entrypoint symlink.${NC}"
fi

# 3. Setup the Thin Bash Directory
echo -e "\n${BLUE}[*] Setting up Thin Bash structure...${NC}"
mkdir -p ~/.config/sentinel
if [[ ! -L ~/.config/sentinel/thin_bashrc ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/thin_bashrc" ~/.config/sentinel/thin_bashrc
    echo -e "    ${GREEN}✔ Symlinked thin_bashrc hook${NC}"
fi
if [[ ! -L ~/.config/sentinel/bash_aliases.d ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/bash_aliases.d" ~/.config/sentinel/bash_aliases.d
    echo -e "    ${GREEN}✔ Symlinked bash_aliases.d${NC}"
fi
if [[ ! -L ~/.config/sentinel/bash_functions.d ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/bash_functions.d" ~/.config/sentinel/bash_functions.d
    echo -e "    ${GREEN}✔ Symlinked bash_functions.d${NC}"
fi
if [[ ! -L ~/.config/sentinel/completions.d ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/completions.d" ~/.config/sentinel/completions.d
    echo -e "    ${GREEN}✔ Symlinked completions.d${NC}"
fi

# 4. Patch .bashrc
echo -e "\n${BLUE}[*] Patching ~/.bashrc...${NC}"
if ! grep -q "sentinel/thin_bashrc" ~/.bashrc; then
    echo -e "\n# SENTINEL Thin Bash" >> ~/.bashrc
    echo "[[ -f ~/.config/sentinel/thin_bashrc ]] && source ~/.config/sentinel/thin_bashrc" >> ~/.bashrc
    echo -e "    ${GREEN}✔ Injected SENTINEL Thin Bash hooks into ~/.bashrc${NC}"
else
    echo -e "    ${YELLOW}⚠ ~/.bashrc already contains SENTINEL hooks.${NC}"
fi

# 5. Trigger WezTerm validation
echo -e "\n${BLUE}[*] Validating WezTerm syntax...${NC}"
if wezterm show-keys &> /dev/null; then
    echo -e "    ${GREEN}✔ WezTerm configuration validated successfully!${NC}"
else
    echo -e "    ${RED}❌ WezTerm validation failed, but installation finished. Check logs.${NC}"
fi

# 6. Build sentinel-core
echo -e "\n${BLUE}[*] Building sentinel-core...${NC}"
if command -v cargo &> /dev/null; then
    echo -e "    ${GREEN}✔ Cargo found. Compiling Rust core...${NC}"
    (cd "${PROJECT_ROOT}/sentinel-core" && cargo build --release)
    echo -e "    ${GREEN}✔ Build successful!${NC}"
else
    echo -e "    ${YELLOW}⚠ Cargo not found. Skipping sentinel-core build.${NC}"
fi

echo -e "\n${CYAN}======================================================${NC}"
echo -e "${CYAN}  BOOTSTRAP COMPLETE! SENTINEL IS READY FOR ACTION.   ${NC}"
echo -e "${CYAN}======================================================${NC}"
echo -e "${GREEN}👉 Restart your terminal or launch 'wezterm' to begin.${NC}\n"
