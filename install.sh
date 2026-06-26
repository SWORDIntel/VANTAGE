#!/usr/bin/env bash
###############################################################################
# VANTAGE - The Hyper-Converged Terminal OS Installer (Unified TUI)
# Usage:
#   ./install.sh              # Interactive TUI menu
#   ./install.sh install      # Direct install (WezTerm pathway)
#   ./install.sh kitty        # Direct install (Kitty pathway)
#   ./install.sh reinstall    # Uninstall + install
#   ./install.sh uninstall    # Remove VANTAGE
###############################################################################

set -euo pipefail

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
KEEP_STATE=0

# ── Helpers ──

header() {
    echo -e "${CYAN}======================================================${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}======================================================${NC}\n"
}

step()  { echo -e "  ${BLUE}[*]${NC} $1"; }
ok()    { echo -e "      ${GREEN}✔${NC} $1"; }
warn()  { echo -e "      ${YELLOW}⚠${NC} $1"; }
fail()  { echo -e "      ${RED}❌${NC} $1"; }

# ── Install (WezTerm pathway) ──

do_install() {
    header "VANTAGE INSTALL (WezTerm Pathway)"

    step "Bootstrapping system dependencies..."
    sudo apt-get update -y || true
    sudo apt-get install -y build-essential clang libbpf-dev zstd curl wget make \
        liburing-dev libluajit-5.1-dev libssl-dev libxdp-dev python3-dev || {
        warn "Could not install some APT dependencies. Continuing anyway..."
    }
    ok "Dependencies installed"

    step "Checking Rust/Cargo..."
    if ! command -v cargo &> /dev/null; then
        warn "Cargo not found. Installing Rust..."
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
        source "$HOME/.cargo/env"
    else
        ok "Rust/Cargo found"
    fi

    step "Checking WezTerm..."
    if ! command -v wezterm &> /dev/null; then
        warn "WezTerm not found. Attempting to install..."
        curl -fsSL https://apt.fury.io/wez/gpg.key | sudo gpg --yes --dearmor -o /usr/share/keyrings/wezterm-fury.gpg
        echo 'deb [signed-by=/usr/share/keyrings/wezterm-fury.gpg] https://apt.fury.io/wez/ * *' | sudo tee /etc/apt/sources.list.d/wezterm.list
        sudo apt-get update -y
        sudo apt-get install -y wezterm || fail "Failed to install WezTerm!"
    else
        ok "WezTerm found"
    fi

    step "Setting up WezTerm Lua architecture..."
    mkdir -p ~/.config/wezterm/plugins
    if [[ ! -L ~/.config/wezterm/plugins/vantage.wezterm ]]; then
        ln -sfn "${PROJECT_ROOT}" ~/.config/wezterm/plugins/vantage.wezterm
        ok "Symlinked ~/.config/wezterm/plugins/vantage.wezterm"
    fi
    if [[ ! -f ~/.wezterm.lua && ! -L ~/.wezterm.lua ]]; then
        ln -sfn "${PROJECT_ROOT}/wezterm_config/wezterm.lua" ~/.wezterm.lua
        ok "Symlinked ~/.wezterm.lua entrypoint"
    fi

    step "Setting up Thin Bash structure..."
    mkdir -p ~/.config/vantage
    if [[ ! -L ~/.config/vantage/thin_bashrc ]]; then
        ln -sfn "${PROJECT_ROOT}/thin_bash/thin_bashrc" ~/.config/vantage/thin_bashrc
        ok "Symlinked thin_bashrc hook"
    fi

    step "Patching ~/.bashrc..."
    if ! grep -q "vantage/thin_bashrc" ~/.bashrc; then
        echo -e "\n# VANTAGE Thin Bash" >> ~/.bashrc
        echo "[[ -f ~/.config/vantage/thin_bashrc ]] && source ~/.config/vantage/thin_bashrc" >> ~/.bashrc
        ok "Injected VANTAGE Thin Bash hooks into ~/.bashrc"
    else
        ok "bashrc already patched"
    fi

    step "Initializing Git Submodules (QIHSE)..."
    cd "${PROJECT_ROOT}"
    git submodule update --init --recursive
    ok "Submodules initialized"

    step "Building QIHSE for current silicon..."
    cd "${PROJECT_ROOT}/vendor/QIHSE"
    if [[ -f scripts/build-native.sh ]]; then
        ./scripts/build-native.sh --auto --verbose
        ok "QIHSE built with silicon-optimized flags"
    else
        warn "build-native.sh not found, falling back to plain make..."
        make lib || true
    fi
    if [[ -f libqihse.so ]]; then
        ok "libqihse.so ready at vendor/QIHSE/libqihse.so"
    else
        fail "libqihse.so not built! QIHSE integration will be limited."
        warn "You may need to install additional dependencies."
    fi

    step "End-to-End Build (Core, Indexer, eBPF)..."
    cd "${PROJECT_ROOT}"
    make core indexer ebpf
    ok "Build successful"

    echo -e "\n${CYAN}======================================================${NC}"
    echo -e "${CYAN}  BOOTSTRAP COMPLETE! VANTAGE IS READY FOR ACTION.    ${NC}"
    echo -e "${CYAN}======================================================${NC}"
    echo -e "${GREEN}Restart your terminal or launch 'wezterm' to begin.${NC}\n"
}

# ── Install (Kitty pathway) ──

do_install_kitty() {
    header "VANTAGE INSTALL (Kitty Pathway)"

    export VANTAGE_KITTY_PRIMARY_CLI=1
    export VANTAGE_SKIP_BLESH=1
    export VANTAGE_SKIP_WAVE=1

    if [[ -f "${PROJECT_ROOT}/legacy_bash/installer/lib/init.sh" ]]; then
        source "${PROJECT_ROOT}/legacy_bash/installer/lib/init.sh"
        source "${PROJECT_ROOT}/legacy_bash/installer/lib/preflight.sh"
        source "${PROJECT_ROOT}/legacy_bash/installer/lib/install_kitty_core.sh"
        source "${PROJECT_ROOT}/legacy_bash/installer/lib/finalize.sh"
    else
        fail "Kitty installer libs not found at legacy_bash/installer/lib/"
        exit 1
    fi
}

# ── Uninstall ──

do_uninstall() {
    header "VANTAGE UNINSTALL"

    if [[ $KEEP_STATE -eq 1 ]]; then
        warn "Running in KEEP-STATE mode. Logs and state.json will be preserved."
    else
        warn "Running in FULL PURGE mode. ALL data will be deleted."
    fi

    step "Removing WezTerm configurations..."
    if [[ -L ~/.config/wezterm/plugins/vantage.wezterm ]]; then
        rm ~/.config/wezterm/plugins/vantage.wezterm
        ok "Removed ~/.config/wezterm/plugins/vantage.wezterm symlink"
    fi
    if [[ -L ~/.config/wezterm/vantage ]]; then
        rm ~/.config/wezterm/vantage
        ok "Removed legacy ~/.config/wezterm/vantage symlink"
    fi
    if [[ -L ~/.wezterm.lua ]]; then
        rm ~/.wezterm.lua
        ok "Removed ~/.wezterm.lua symlink"
    fi

    step "Unpatching ~/.bashrc..."
    if grep -q "vantage/thin_bashrc" ~/.bashrc; then
        sed -i '/# VANTAGE Thin Bash/d' ~/.bashrc
        sed -i '/\[\[ -f ~\/.config\/vantage\/thin_bashrc \]\] && source ~\/.config\/vantage\/thin_bashrc/d' ~/.bashrc
        ok "Removed VANTAGE source hooks from ~/.bashrc"
    else
        warn "No VANTAGE hooks found in ~/.bashrc"
    fi

    step "Cleaning up ~/.config/vantage directory..."
    if [[ -d ~/.config/vantage ]]; then
        if [[ $KEEP_STATE -eq 1 ]]; then
            rm -rf ~/.config/vantage/bash_aliases.d
            rm -rf ~/.config/vantage/bash_functions.d
            rm -rf ~/.config/vantage/completions.d
            rm -f ~/.config/vantage/thin_bashrc
            ok "Removed bash scripts but preserved state.json and logs."
        else
            rm -rf ~/.config/vantage
            rm -rf ~/.cache/vantage 2>/dev/null || true
            ok "Fully purged ~/.config/vantage and caches."
        fi
    else
        warn "~/.config/vantage already removed."
    fi

    echo -e "\n${GREEN}======================================================${NC}"
    echo -e "${GREEN}  UNINSTALL COMPLETE! VANTAGE HAS BEEN REMOVED.      ${NC}"
    echo -e "${GREEN}======================================================${NC}"
}

# ── Reinstall ──

do_reinstall() {
    header "VANTAGE REINSTALL"
    do_uninstall
    echo ""
    do_install
    echo -e "\n${CYAN}======================================================${NC}"
    echo -e "${CYAN}      REINSTALLATION COMPLETE! SYSTEM IS FRESH.       ${NC}"
    echo -e "${CYAN}======================================================${NC}"
}

# ── TUI Menu ──

show_menu() {
    echo -e "${CYAN}======================================================${NC}"
    echo -e "${CYAN}     VANTAGE - Hyper-Converged Terminal OS Installer   ${NC}"
    echo -e "${CYAN}======================================================${NC}"
    echo ""
    echo -e "  ${BOLD}1)${NC} Install (WezTerm pathway)"
    echo -e "  ${BOLD}2)${NC} Install (Kitty pathway)"
    echo -e "  ${BOLD}3)${NC} Reinstall (uninstall + install)"
    echo -e "  ${BOLD}4)${NC} Uninstall"
    echo -e "  ${BOLD}5)${NC} Uninstall (keep state/logs)"
    echo -e "  ${BOLD}q)${NC} Quit"
    echo ""
    read -rp "Select an option [1-5/q]: " choice
    case "$choice" in
        1) do_install ;;
        2) do_install_kitty ;;
        3) do_reinstall ;;
        4) do_uninstall ;;
        5) KEEP_STATE=1; do_uninstall ;;
        q|Q) echo "Aborted."; exit 0 ;;
        *) echo "Invalid option."; exit 1 ;;
    esac
}

# ── Entry point ──

# Parse --keep-state for uninstall subcommand
for arg in "$@"; do
    if [[ "$arg" == "--keep-state" ]]; then
        KEEP_STATE=1
    fi
done

case "${1:-menu}" in
    install)    do_install ;;
    kitty)      do_install_kitty ;;
    reinstall)  do_reinstall ;;
    uninstall)  do_uninstall ;;
    menu|"")    show_menu ;;
    -h|--help)
        echo "Usage: ./install.sh [install|kitty|reinstall|uninstall] [--keep-state]"
        echo ""
        echo "  install    - Install VANTAGE (WezTerm pathway)"
        echo "  kitty      - Install VANTAGE (Kitty pathway)"
        echo "  reinstall  - Uninstall then fresh install"
        echo "  uninstall  - Remove VANTAGE (--keep-state to preserve logs)"
        echo "  (none)     - Interactive TUI menu"
        ;;
    *)
        echo "Unknown command: $1"
        echo "Usage: ./install.sh [install|kitty|reinstall|uninstall] [--keep-state]"
        exit 1
        ;;
esac
