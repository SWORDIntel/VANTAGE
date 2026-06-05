#!/usr/bin/env bash
###############################################################################
# SENTINEL - WezTerm Edition Reinstaller
# -----------------------------------------------
# This script cleanly uninstalls and then immediately reinstalls the framework.
###############################################################################

set -euo pipefail

# ANSI Colors for Rich Feedback
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

echo -e "${CYAN}======================================================${NC}"
echo -e "${CYAN}         SENTINEL WEZTERM EDITION REINSTALLER         ${NC}"
echo -e "${CYAN}======================================================${NC}\n"

echo -e "${GREEN}[*] Initiating Uninstallation Phase...${NC}\n"
"${PROJECT_ROOT}/uninstall.sh"

echo -e "\n${GREEN}[*] Initiating Installation Phase...${NC}\n"
"${PROJECT_ROOT}/install.sh"

echo -e "\n${CYAN}======================================================${NC}"
echo -e "${CYAN}      REINSTALLATION COMPLETE! SYSTEM IS FRESH.       ${NC}"
echo -e "${CYAN}======================================================${NC}"