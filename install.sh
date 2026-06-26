#!/usr/bin/env bash
###############################################################################
# VANTAGE - The Hyper-Converged Terminal OS Installer
###############################################################################

set -euo pipefail

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

echo -e "${CYAN}======================================================${NC}"
echo -e "${CYAN}          VANTAGE WEZTERM EDITION INSTALLER           ${NC}"
echo -e "${CYAN}======================================================${NC}\n"

# 1. Bootstrap System Dependencies
echo -e "${BLUE}[*] Bootstrapping System Dependencies...${NC}"
sudo apt-get update -y || true
sudo apt-get install -y build-essential clang libbpf-dev zstd curl wget make || {
    echo -e "    ${YELLOW}⚠ Could not install some APT dependencies. Continuing anyway...${NC}"
}

# 2. Bootstrap Rust (if missing)
echo -e "${BLUE}[*] Checking Rust/Cargo...${NC}"
if ! command -v cargo &> /dev/null; then
    echo -e "    ${YELLOW}⚠ Cargo not found. Installing Rust...${NC}"
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source "$HOME/.cargo/env"
else
    echo -e "    ${GREEN}✔ Rust/Cargo found!${NC}"
fi

# 3. Ensure WezTerm is installed
echo -e "${BLUE}[*] Checking WezTerm...${NC}"
if ! command -v wezterm &> /dev/null; then
    echo -e "    ${YELLOW}⚠ WezTerm not found. Attempting to install...${NC}"
    curl -fsSL https://apt.fury.io/wez/gpg.key | sudo gpg --yes --dearmor -o /usr/share/keyrings/wezterm-fury.gpg
    echo 'deb [signed-by=/usr/share/keyrings/wezterm-fury.gpg] https://apt.fury.io/wez/ * *' | sudo tee /etc/apt/sources.list.d/wezterm.list
    sudo apt-get update -y
    sudo apt-get install -y wezterm || echo -e "    ${RED}❌ Failed to install WezTerm!${NC}"
else
    echo -e "    ${GREEN}✔ WezTerm found!${NC}"
fi

# 4. Setup WezTerm Configuration
echo -e "\n${BLUE}[*] Setting up native WezTerm Lua architecture...${NC}"
mkdir -p ~/.config/wezterm/plugins
if [[ ! -L ~/.config/wezterm/plugins/vantage.wezterm ]]; then
    ln -sfn "${PROJECT_ROOT}" ~/.config/wezterm/plugins/vantage.wezterm
    echo -e "    ${GREEN}✔ Symlinked ~/.config/wezterm/plugins/vantage.wezterm${NC}"
fi

if [[ ! -f ~/.wezterm.lua && ! -L ~/.wezterm.lua ]]; then
    ln -sfn "${PROJECT_ROOT}/wezterm_config/wezterm.lua" ~/.wezterm.lua
    echo -e "    ${GREEN}✔ Symlinked ~/.wezterm.lua entrypoint${NC}"
fi

# 5. Setup the Thin Bash Directory
echo -e "\n${BLUE}[*] Setting up Thin Bash structure...${NC}"
mkdir -p ~/.config/vantage
if [[ ! -L ~/.config/vantage/thin_bashrc ]]; then
    ln -sfn "${PROJECT_ROOT}/thin_bash/thin_bashrc" ~/.config/vantage/thin_bashrc
    echo -e "    ${GREEN}✔ Symlinked thin_bashrc hook${NC}"
fi

# 6. Patch .bashrc
echo -e "\n${BLUE}[*] Patching ~/.bashrc...${NC}"
if ! grep -q "vantage/thin_bashrc" ~/.bashrc; then
    echo -e "\n# VANTAGE Thin Bash" >> ~/.bashrc
    echo "[[ -f ~/.config/vantage/thin_bashrc ]] && source ~/.config/vantage/thin_bashrc" >> ~/.bashrc
    echo -e "    ${GREEN}✔ Injected VANTAGE Thin Bash hooks into ~/.bashrc${NC}"
fi

# 7. End-to-End Build (Core, Indexer, eBPF)
echo -e "\n${BLUE}[*] Executing full end-to-end build...${NC}"
cd "${PROJECT_ROOT}"
make all
echo -e "    ${GREEN}✔ Build successful!${NC}"

echo -e "\n${CYAN}======================================================${NC}"
echo -e "${CYAN}  BOOTSTRAP COMPLETE! VANTAGE IS READY FOR ACTION.    ${NC}"
echo -e "${CYAN}======================================================${NC}"
echo -e "${GREEN}👉 Restart your terminal or launch 'wezterm' to begin.${NC}\n"
