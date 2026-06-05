#!/usr/bin/env bash
# SENTINEL Core Aliases
# Enhanced shell aliases for improved productivity and security

# Load additional alias files from directory

# Basic utilities
alias q='exit'                  # Simple alias to exit the terminal
alias rebash='exec bash -l'     # Reload bash configuration

# Color support for common commands (if color support is enabled)
alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'
alias diff='diff --color=auto'
alias ip='ip -color=auto'

# File operations
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'
alias ls-readdir='ls --color=none --format commas'
alias l1='ls -1'
alias la1='ls -a -1'

# Navigation
alias ..='cd ..'
alias ...='cd ../..'
alias ....='cd ../../..'

# System information
alias sysinfo='echo "CPU:"; lscpu | grep "Model name"; echo -e "\nMemory:"; free -h; echo -e "\nDisk:"; df -h'
alias meminfo='free -h'
alias cpuinfo='lscpu'

# Network utilities
alias myip='curl -s https://ipinfo.io/ip'
alias ports='netstat -tulanp'
alias iptables-list='sudo iptables -L -n -v --line-numbers'
alias check-listening='netstat -plunt'
alias open-ports='ss -tulpn'

# Process management
alias psg='ps aux | grep -v grep | grep -i -e VSZ -e'

# Git shortcuts
alias gs='git status'
alias gd='git diff'
alias gl='git log --oneline --graph --decorate --all'

# System updates (cross-distribution)
alias update-system='if command -v apt &>/dev/null; then
                       sudo apt update && sudo apt upgrade -y;
                     elif command -v dnf &>/dev/null; then
                       sudo dnf upgrade -y;
                     elif command -v yum &>/dev/null; then
                       sudo yum update -y;
                     elif command -v pacman &>/dev/null; then
                       sudo pacman -Syu;
                     fi'

# Sourcing with optional internet check
alias sourcebash='source ~/.bashrc && check_internet_and_update && echo ".bashrc sourced and apt updated if connected."'
alias sentinel-config='~/.sentinel/sentinel_config_helper.sh'

# Monero unified wallet
export MONERO_WALLET="$HOME/Monero/wallets/MyWallet/MyWallet"
alias monero-gui="~/Programs/monero-gui/monero-wallet-gui"
alias monero-cli="~/Programs/monero-cli/monero-wallet-cli --wallet-file $MONERO_WALLET"
alias monerod="~/Programs/monero-cli/monerod --prune-blockchain"
