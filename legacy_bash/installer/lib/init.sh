#!/usr/bin/env bash
# SENTINEL Installer - Initialization

# Strict mode to catch errors
set -euo pipefail

# Define critical variables
PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
if [[ -z "${INSTALLER_VERSION:-}" ]]; then
    if INSTALLER_VERSION="$(git -C "$PROJECT_ROOT" describe --tags --always 2>/dev/null)"; then
        :
    else
        INSTALLER_VERSION="unknown"
    fi
fi
export INSTALLER_VERSION
LOG_DIR="${HOME}/logs"
STATE_FILE="${HOME}/install.state"
ROLLBACK_SCRIPT="${HOME}/.sentinel_rollback.sh"
BLESH_DIR="${HOME}/.local/share/blesh"
BLESH_LOADER="${HOME}/blesh_loader.sh"
MODULES_DIR="${HOME}/bash_modules.d"

# Source helper scripts
# shellcheck source=installer/helpers.sh
source "${PROJECT_ROOT}/installer/helpers.sh"
# shellcheck source=installer/dependencies.sh
source "${PROJECT_ROOT}/installer/dependencies.sh"
# shellcheck source=installer/directories.sh
source "${PROJECT_ROOT}/installer/directories.sh"
# shellcheck source=installer/python.sh
source "${PROJECT_ROOT}/installer/python.sh"
# shellcheck source=installer/blesh.sh
source "${PROJECT_ROOT}/installer/blesh.sh"
# shellcheck source=installer/bash.sh
source "${PROJECT_ROOT}/installer/bash.sh"
# shellcheck source=installer/kitty.sh
source "${PROJECT_ROOT}/installer/kitty.sh"

# Ensure logs directory exists before any logging
if [[ ! -d "$LOG_DIR" ]]; then
    mkdir -p "$LOG_DIR"
fi

# Error handler
trap 'fail "Installer aborted on line $LINENO; see ${LOG_DIR}/install.log"' ERR

# State management functions
mark_done()  { echo "$1" >> "${STATE_FILE}"; }
is_done()    { grep -qxF "$1" "${STATE_FILE:-/dev/null}" 2>/dev/null; }
