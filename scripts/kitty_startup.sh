#!/usr/bin/env bash
# VANTAGE Kitty Startup Script
# This script is executed when kitty starts a new shell session
# It ensures VANTAGE modules are loaded correctly in kitty

# Mark that we're starting in kitty
export VANTAGE_KITTY_PRIMARY_CLI=1
export VANTAGE_TERMINAL="kitty"

# Source kitty.rc if it exists
if [[ -f "${HOME}/kitty.rc" ]]; then
    source "${HOME}/kitty.rc"
fi

# Ensure kitty integration module is loaded early
if [[ -f "${HOME}/bash_modules.d/kitty_integration.module" ]]; then
    source "${HOME}/bash_modules.d/kitty_integration.module"
elif [[ -f "${VANTAGE_ROOT}/bash_modules.d/kitty_integration.module" ]]; then
    source "${VANTAGE_ROOT}/bash_modules.d/kitty_integration.module"
fi

# Set kitty window title
if [[ -n "${KITTY_WINDOW_ID:-}" ]] && type vantage_kitty_set_title &>/dev/null; then
    vantage_kitty_set_title "VANTAGE - $(basename "${PWD}")"
fi
