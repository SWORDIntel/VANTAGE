#!/usr/bin/env bash
###############################################################################
# SENTINEL - WezTerm Edition Uninstaller
# -----------------------------------------------
# This script removes the SENTINEL WezTerm architecture, Thin Bash
# integration, and cleans up .bashrc.
###############################################################################

set -euo pipefail

# ANSI Colors for Rich Feedback
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

KEEP_STATE=0
for arg in "$@"; do
    if [[ "$arg" == "--keep-state" ]]; then
        KEEP_STATE=1
    fi
done

echo -e "${BLUE}======================================================${NC}"
echo -e "${BLUE}        SENTINEL WEZTERM EDITION UNINSTALLER          ${NC}"
echo -e "${BLUE}======================================================${NC}"

if [[ $KEEP_STATE -eq 1 ]]; then
    echo -e "${YELLOW}[!] Running in KEEP-STATE mode. Logs and state.json will be preserved.${NC}\n"
else
    echo -e "${RED}[!] Running in FULL PURGE mode. ALL data will be deleted.${NC}\n"
fi

# 1. Remove WezTerm Lua Symlinks
echo -e "${BLUE}[*] Removing WezTerm configurations...${NC}"
if [[ -L ~/.config/wezterm/plugins/sentinel.wezterm ]]; then
    rm ~/.config/wezterm/plugins/sentinel.wezterm
    echo -e "    ${GREEN}✔ Removed ~/.config/wezterm/plugins/sentinel.wezterm symlink${NC}"
fi

if [[ -L ~/.wezterm.lua ]]; then
    rm ~/.wezterm.lua
    echo -e "    ${GREEN}✔ Removed ~/.wezterm.lua symlink${NC}"
fi

# 2. Unpatch .bashrc
echo -e "${BLUE}[*] Unpatching ~/.bashrc...${NC}"
if grep -q "sentinel/thin_bashrc" ~/.bashrc; then
    sed -i '/# SENTINEL Thin Bash/d' ~/.bashrc
    sed -i '/\[\[ -f ~\/.config\/sentinel\/thin_bashrc \]\] && source ~\/.config\/sentinel\/thin_bashrc/d' ~/.bashrc
    echo -e "    ${GREEN}✔ Removed SENTINEL source hooks from ~/.bashrc${NC}"
else
    echo -e "    ${YELLOW}⚠ No SENTINEL hooks found in ~/.bashrc${NC}"
fi

# 3. Clean up Thin Bash directory
echo -e "${BLUE}[*] Cleaning up ~/.config/sentinel directory...${NC}"
if [[ -d ~/.config/sentinel ]]; then
    if [[ $KEEP_STATE -eq 1 ]]; then
        # Remove only the bash scripts
        rm -rf ~/.config/sentinel/bash_aliases.d
        rm -rf ~/.config/sentinel/bash_functions.d
        rm -rf ~/.config/sentinel/completions.d
        rm -f ~/.config/sentinel/thin_bashrc
        echo -e "    ${GREEN}✔ Removed bash scripts but preserved state.json and logs.${NC}"
    else
        # Full purge
        rm -rf ~/.config/sentinel
        rm -rf ~/.cache/sentinel 2>/dev/null || true
        echo -e "    ${GREEN}✔ Fully purged ~/.config/sentinel and caches.${NC}"
    fi
else
    echo -e "    ${YELLOW}⚠ ~/.config/sentinel already removed.${NC}"
fi

echo -e "\n${GREEN}======================================================${NC}"
echo -e "${GREEN}  UNINSTALL COMPLETE! SENTINEL HAS BEEN REMOVED.      ${NC}"
echo -e "${GREEN}======================================================${NC}"