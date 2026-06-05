#!/usr/bin/env bash
# SENTINEL Installer - Main Entry Point

# Source the initialization script
source "$(dirname "${BASH_SOURCE[0]}")/lib/init.sh"

# Handle --dry-run flag
DRY_RUN=0
INTERACTIVE="${INTERACTIVE:-1}"

# Parse flags that affect control flow before any prompts.
for arg in "$@"; do
    case "$arg" in
        --dry-run)
            DRY_RUN=1
            ;;
        --non-interactive)
            INTERACTIVE=0
            ;;
        --headless)
            export SENTINEL_HEADLESS=1
            export SENTINEL_SKIP_BLESH=1
            export SENTINEL_SKIP_WAVE=1
            ;;
    esac
done

# Detect installation pathway
INSTALL_PATHWAY="${SENTINEL_INSTALL_PATHWAY:-bash}"

# Check for kitty pathway flag
if [[ " $@ " =~ " --kitty-primary " ]] || [[ "${INSTALL_PATHWAY}" == "kitty" ]]; then
    export SENTINEL_KITTY_PRIMARY_CLI=1
    export SENTINEL_SKIP_BLESH=1
    export SENTINEL_SKIP_WAVE=1
    INSTALL_PATHWAY="kitty"
    log "Selected installation pathway: Kitty Primary CLI"
elif [[ $INTERACTIVE -eq 1 ]] && [[ "${SENTINEL_HEADLESS:-0}" != "1" ]]; then
    # Interactive mode: ask user which pathway
    echo ""
    echo "SENTINEL Installation Pathway Selection"
    echo "========================================"
    echo "1. Bash (default) - Traditional bash-based installation"
    echo "2. Kitty Primary CLI - GPU-accelerated terminal as primary CLI"
    echo ""
    read -r -t 30 -p "Select installation pathway [1-2] (default: 1): " pathway_choice || pathway_choice="1"
    
    case "$pathway_choice" in
        2|kitty|Kitty)
            export SENTINEL_KITTY_PRIMARY_CLI=1
            export SENTINEL_SKIP_BLESH=1
            export SENTINEL_SKIP_WAVE=1
            INSTALL_PATHWAY="kitty"
            log "User selected: Kitty Primary CLI pathway"
            ;;
        *)
            INSTALL_PATHWAY="bash"
            log "User selected: Bash pathway (default)"
            ;;
    esac
else
    INSTALL_PATHWAY="bash"
    log "Using default installation pathway: Bash"
fi

# Source the pre-flight checks script
source "${PROJECT_ROOT}/installer/lib/preflight.sh"

# Source the appropriate core installation script
if [ $DRY_RUN -eq 0 ]; then
    if [[ "${INSTALL_PATHWAY}" == "kitty" ]]; then
        source "${PROJECT_ROOT}/installer/lib/install_kitty_core.sh"
    else
        source "${PROJECT_ROOT}/installer/lib/install_core.sh"
    fi
fi

# Source the finalization script
if [ $DRY_RUN -eq 0 ]; then
    source "${PROJECT_ROOT}/installer/lib/finalize.sh"
fi
