#!/usr/bin/env bash
# SENTINEL Installer - Helper Functions

# Colour helpers (safe on dark backgrounds and TTY-agnostic)
init_colour_palette() {
    local force_colour="${FORCE_COLOR:-}" # honour common tooling flag
    local disable_colour="${NO_COLOR:-}"  # standard opt-out

    # Default to no colour until proven safe
    c_red=""; c_green=""; c_yellow=""; c_blue=""; c_reset=""

    # Skip colouring on non-TTY output unless explicitly forced
    if [[ ! -t 1 && -z "$force_colour" ]]; then
        return
    fi

    # Honour NO_COLOR request
    if [[ -n "$disable_colour" ]]; then
        return
    fi

    local colour_capability
    colour_capability=$(tput colors 2>/dev/null || echo 0)
    if [[ -z "$force_colour" && "$colour_capability" -lt 8 ]]; then
        return
    fi

    # High-contrast palette tuned for black/dark backgrounds
    c_red=$'\033[1;91m'
    c_green=$'\033[1;92m'
    c_yellow=$'\033[1;93m'
    c_blue=$'\033[1;96m'
    c_reset=$'\033[0m'
}

init_colour_palette

# Safe operation wrappers
safe_rsync() {
    if ! rsync "$@"; then
        fail "rsync operation failed: $*"
    fi
}

safe_cp() {
    if ! cp "$@"; then
        fail "cp operation failed: $*"
    fi
}

safe_mkdir() {
    local dir="$1"
    local perm="${2:-700}"  # Default permission 700 (user rwx only)

    if ! mkdir -p "$dir"; then
        fail "Failed to create directory: $dir"
    fi

    # Set proper permissions
    chmod "$perm" "$dir" 2>/dev/null || {
        warn "Failed to set permissions $perm on directory: $dir"
    }

    ok "Created directory: $dir with permissions: $perm"
    return 0
}

# Robust error handler for fatal errors (security: prevents silent failures)
fail() {
    # Telemetry must never block installer shutdown (outbound-only; drop-on-fail).
    if type -t installer_fabric_emit_event >/dev/null 2>&1; then
        installer_fabric_emit_event "installer_complete" "{\"status\":\"fail\",\"version\":\"${INSTALLER_VERSION:-unknown}\"}" || true
    fi
    echo "${c_red}✖${c_reset}  $*" | tee -a "${LOG_DIR}/install.log" >&2
    echo "Run '${ROLLBACK_SCRIPT}' to restore previous configuration" >&2
    exit 1
}

# Success logger for status lines (security: ensures auditability)
ok() {
    echo "${c_green}✔${c_reset}  $*" | tee -a "${LOG_DIR}/install.log"
}

# Progress step logger for status lines (security: ensures auditability)
step() {
    echo "${c_blue}→${c_reset}  $*" | tee -a "${LOG_DIR}/install.log"
}

# Warning logger for non-fatal issues (security: ensures visibility of issues)
warn() {
    echo "${c_yellow}!${c_reset}  $*" | tee -a "${LOG_DIR}/install.log" >&2
}

# Enhanced logging with timestamp
log() {
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local log_file="${LOG_DIR}/install.log"
    echo "[$timestamp] $*" | tee -a "$log_file"
}

# -----------------------------------------------------------------------------
# Fabric telemetry (optional, outbound-only, non-blocking, drop-on-fail)
# -----------------------------------------------------------------------------
_installer_fabric_bool() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) echo 1 ;;
        *) echo 0 ;;
    esac
}

_installer_fabric_json_escape() {
    local s="${1:-}"
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    s="${s//$'\n'/ }"
    s="${s//$'\r'/ }"
    s="${s//$'\t'/ }"
    printf '%s' "$s"
}

installer_fabric_emit_event() {
    # installer_fabric_emit_event <type> <payload_json>
    # IMPORTANT: Keep this best-effort and fast; never block shell startup.
    local event_type="${1:-}"
    local payload_json="${2:-{}}"

    local fabric_enabled telemetry_enabled
    fabric_enabled="$(_installer_fabric_bool "${FABRIC_ENABLED:-0}")"
    telemetry_enabled="$(_installer_fabric_bool "${FABRIC_TELEMETRY_ENABLED:-0}")"
    [[ "$fabric_enabled" == "1" && "$telemetry_enabled" == "1" ]] || return 0

    [[ -n "$event_type" && "$event_type" =~ ^[A-Za-z0-9_.-]+$ ]] || return 0
    payload_json="$(printf '%s' "${payload_json:-{}}" | tr -d '\n' | tr -d '\r')"

    local timeout_ms transport socket endpoint
    timeout_ms="${FABRIC_TELEMETRY_TIMEOUT_MS:-50}"
    transport="${FABRIC_TELEMETRY_TRANSPORT:-unix}"
    socket="${FABRIC_TELEMETRY_UNIX_SOCKET:-}"
    endpoint="${FABRIC_TELEMETRY_HTTP_ENDPOINT:-}"

    local ts
    ts="$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S')"

    local node_id device_id layer_id
    node_id="${FABRIC_IDENTITY_NODE_ID:-}"
    device_id="${FABRIC_IDENTITY_DEVICE_ID:-}"
    layer_id="${FABRIC_IDENTITY_LAYER_ID:-}"

    local line
    line=$(
        printf '{"ts":"%s","type":"%s","identity":{"node_id":"%s","device_id":"%s","layer_id":"%s"},"payload":%s}\n' \
            "$(_installer_fabric_json_escape "$ts")" \
            "$(_installer_fabric_json_escape "$event_type")" \
            "$(_installer_fabric_json_escape "$node_id")" \
            "$(_installer_fabric_json_escape "$device_id")" \
            "$(_installer_fabric_json_escape "$layer_id")" \
            "$payload_json"
    )

    local timeout_s="0.05"
    if [[ "$timeout_ms" =~ ^[0-9]+$ ]]; then
        timeout_s="0.$(printf '%03d' "$timeout_ms")"
        if (( timeout_ms >= 1000 )); then
            timeout_s="$((timeout_ms / 1000))"
        fi
    fi

    case "$transport" in
        unix)
            [[ -n "$socket" ]] || return 0
            if command -v timeout >/dev/null 2>&1 && command -v nc >/dev/null 2>&1; then
                (timeout "$timeout_s" nc -U "$socket" >/dev/null 2>&1 <<<"$line" || true) >/dev/null 2>&1 &
            elif command -v timeout >/dev/null 2>&1 && command -v socat >/dev/null 2>&1; then
                (timeout "$timeout_s" socat - "UNIX-CONNECT:${socket}" >/dev/null 2>&1 <<<"$line" || true) >/dev/null 2>&1 &
            fi
            ;;
        http)
            [[ -n "$endpoint" ]] || return 0
            if command -v curl >/dev/null 2>&1; then
                (
                    curl -sS \
                        --connect-timeout "$timeout_s" \
                        --max-time "$timeout_s" \
                        -H 'Content-Type: application/json' \
                        --data-binary @- \
                        "$endpoint" >/dev/null 2>&1 <<<"$line" || true
                ) >/dev/null 2>&1 &
            fi
            ;;
        *)
            return 0
            ;;
    esac

    return 0
}

# Secure git clone function with integrity checking
safe_git_clone() {
    local depth_arg=""
    local url=""
    local target_dir=""
    local expected_commit=""

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --depth=*)
                depth_arg="--depth=${1#*=}"
                shift
                ;;
            --depth)
                depth_arg="--depth=$2"
                shift 2
                ;;
            --verify-commit=*)
                expected_commit="${1#*=}"
                shift
                ;;
            *)
                if [[ -z "$url" ]]; then
                    url="$1"
                elif [[ -z "$target_dir" ]]; then
                    target_dir="$1"
                fi
                shift
                ;;
        esac
    done

    # Validate inputs
    if [[ -z "$url" || -z "$target_dir" ]]; then
        fail "safe_git_clone: URL and target directory are required"
    fi

    # Validate URL (basic security check)
    if ! [[ "$url" =~ ^https:// ]]; then
        fail "safe_git_clone: Only HTTPS URLs are allowed for security"
    fi

    # Validate target directory
    if [[ "$target_dir" =~ [[:space:]] ]]; then
        fail "safe_git_clone: Target directory cannot contain spaces"
    fi

    # If target exists and is a git repo, try to update it
    if [[ -d "$target_dir/.git" ]]; then
        step "Updating existing repository in $target_dir"
        git -C "$target_dir" fetch origin || fail "Failed to fetch updates"
        git -C "$target_dir" reset --hard origin/HEAD || fail "Failed to reset to origin"

        # Verify commit if specified
        if [[ -n "$expected_commit" ]]; then
            local actual_commit
            actual_commit=$(git -C "$target_dir" rev-parse HEAD)
            if [[ "$actual_commit" != "$expected_commit"* ]]; then
                warn "Repository commit mismatch. Expected: $expected_commit, Got: $actual_commit"
                warn "This may indicate the repository has been updated. Proceeding with caution."
            else
                ok "Repository integrity verified"
            fi
        fi

        return 0
    fi

    # Clone the repository
    step "Cloning $url to $target_dir"
    if [[ -n "$depth_arg" ]]; then
        git clone "$depth_arg" "$url" "$target_dir" || fail "Clone failed"
    else
        git clone "$url" "$target_dir" || fail "Clone failed"
    fi

    # Verify the clone
    if [[ ! -d "$target_dir/.git" ]]; then
        fail "Repository was not cloned correctly"
    fi

    # Verify commit if specified
    if [[ -n "$expected_commit" ]]; then
        local actual_commit
        actual_commit=$(git -C "$target_dir" rev-parse HEAD)
        if [[ "$actual_commit" != "$expected_commit"* ]]; then
            warn "Repository commit mismatch. Expected: $expected_commit, Got: $actual_commit"
            warn "This may indicate the repository has been updated. Proceeding with caution."
        else
            ok "Repository integrity verified"
        fi
    fi

    ok "Repository cloned successfully"
}

# Detect if running in headless/VPS environment
detect_headless_environment() {
    # Multiple detection methods for headless environments
    local is_headless=0

    # Check 1: No DISPLAY variable (common on servers)
    if [[ -z "${DISPLAY:-}" ]]; then
        is_headless=1
    fi

    # Check 2: SSH session without X11 forwarding
    if [[ -n "${SSH_CONNECTION:-}" || -n "${SSH_CLIENT:-}" ]] && [[ -z "${DISPLAY:-}" ]]; then
        is_headless=1
    fi

    # Check 3: Check if running under systemd --user without a display manager
    if systemctl --user is-active --quiet graphical-session.target 2>/dev/null; then
        is_headless=0
    elif [[ -z "${XDG_SESSION_TYPE:-}" ]] || [[ "${XDG_SESSION_TYPE}" == "tty" ]]; then
        is_headless=1
    fi

    # Check 4: No GPU/display devices (common on VPS)
    if ! command -v xrandr &>/dev/null && ! command -v xdpyinfo &>/dev/null; then
        is_headless=1
    fi

    # Check 5: Common VPS/cloud provider detection
    if [[ -f /sys/class/dmi/id/product_name ]]; then
        local product_name
        product_name=$(cat /sys/class/dmi/id/product_name 2>/dev/null || echo "")
        case "$product_name" in
            *"Google Compute Engine"* | *"Droplet"* | *"Amazon EC2"* | *"Virtual Machine"* | *"VirtualBox"* | *"KVM"*)
                is_headless=1
                ;;
        esac
    fi

    # Return headless status
    return $is_headless
}

create_rollback_script() {
    step "Creating rollback script at ${ROLLBACK_SCRIPT}"
    cat > "${ROLLBACK_SCRIPT}" <<'EOF'
#!/bin/bash
# SENTINEL Rollback Script
# Created: $(date)
# This script will restore your system to pre-SENTINEL state

echo "Starting SENTINEL rollback..."

# Restore bashrc from most recent backup
LATEST_BACKUP=$(ls -t "${HOME}"/.bashrc.sentinel.bak.* 2>/dev/null | head -1)
if [[ -n "$LATEST_BACKUP" && -f "$LATEST_BACKUP" ]]; then
    cp "$LATEST_BACKUP" "${HOME}/.bashrc"
    echo "✔ Restored .bashrc from $LATEST_BACKUP"
else
    echo "✖ No .bashrc backup found"
fi

# Remove SENTINEL directories
for dir in bash_modules.d autocomplete logs; do
    if [[ -d "${HOME}/$dir" ]]; then
        rm -rf "${HOME}/$dir"
        echo "✔ Removed $dir"
    fi
done

# Remove SENTINEL files
for file in .bashrc.sentinel bashrc.postcustom blesh_loader.sh .bash_modules install.state; do
    if [[ -f "${HOME}/$file" ]]; then
        rm -f "${HOME}/$file"
        echo "✔ Removed $file"
    fi
done

# Remove Python venv
if [[ -d "${HOME}/venv" ]]; then
    rm -rf "${HOME}/venv"
    echo "✔ Removed Python virtual environment"
fi

# Remove BLE.sh
if [[ -d "${HOME}/.local/share/blesh" ]]; then
    rm -rf "${HOME}/.local/share/blesh"
    echo "✔ Removed BLE.sh"
fi

echo "Rollback complete. Please restart your terminal."
EOF
    chmod 700 "${ROLLBACK_SCRIPT}"
    ok "Rollback script created"
}
