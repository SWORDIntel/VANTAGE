#!/usr/bin/env bash
# VANTAGE Installer - Pre-flight Checks

# Pre-flight checks
check_python_version
PYTHON_CMD=$(find_python) || fail "No suitable Python 3.6+ found"
export PYTHON_CMD

# Allow minimal installs/tests to skip heavy venv dependency installation
if [[ "${VANTAGE_SKIP_PYTHON_VENV:-0}" != "1" ]]; then
    setup_python_venv
else
    log "Skipping Python venv setup (VANTAGE_SKIP_PYTHON_VENV=1)"
fi

# Load configuration
{
    umask 077
    _vantage_config_exports="$(mktemp "${HOME}/.vantage_config_exports.XXXXXX")"
    _vantage_cfg_py="${HOME}/venv/bin/python"
    if [[ ! -x "${_vantage_cfg_py}" ]]; then
        _vantage_cfg_py="${PYTHON_CMD}"
    fi
    "${_vantage_cfg_py}" "${PROJECT_ROOT}/installer/config.py" --output "${_vantage_config_exports}" || fail "Failed to parse configuration"
    # shellcheck disable=SC1090
    source "${_vantage_config_exports}"
    rm -f "${_vantage_config_exports}" 2>/dev/null || true
}

# Detect headless environment and auto-configure
if detect_headless_environment; then
    export VANTAGE_HEADLESS=1
    export VANTAGE_SKIP_BLESH=1
    export VANTAGE_SKIP_WAVE=1
    step "Headless VPS environment detected"
    log "Auto-configuring for server environment (no GUI)"
    log "BLE.sh and Wave Terminal configuration will be skipped"
else
    export VANTAGE_HEADLESS=0
    log "GUI environment detected - enabling full features"
fi

# Unattended install flag
INTERACTIVE=1
for arg in "$@"; do
  case "$arg" in
    --non-interactive)
      INTERACTIVE=0
      ;;
    --headless)
      VANTAGE_HEADLESS=1
      VANTAGE_SKIP_BLESH=1
      VANTAGE_SKIP_WAVE=1
      ;;
  esac
done

# Auto-enable non-interactive mode in headless environments
if [[ "${VANTAGE_HEADLESS}" == "1" ]] && [[ "${INTERACTIVE}" == "1" ]]; then
    log "Headless environment detected - enabling non-interactive mode"
    INTERACTIVE=0
fi

# Create rollback script before any changes
create_rollback_script

# Run dependency checks
check_dependencies
check_platform_dependencies
