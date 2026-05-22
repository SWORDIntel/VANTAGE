#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"

fail() { echo "FAIL: $*" >&2; exit 1; }

tmp_home="$(mktemp -d)"
trap 'rm -rf "$tmp_home"' EXIT

export HOME="$tmp_home"
export XDG_CONFIG_HOME="$HOME/.config"
export SENTINEL_SKIP_PYTHON_VENV=1

# Ensure installer sees "headless" and does not attempt GUI/kitty activation
unset DISPLAY WAYLAND_DISPLAY

echo "[test] temp HOME=$HOME"

# Create an initial writable bashrc so patching is deterministic
mkdir -p "$HOME"
touch "$HOME/.bashrc"
chmod 600 "$HOME/.bashrc"

run_install() {
  bash "${PROJECT_ROOT}/install.sh" --non-interactive --headless >/dev/null
}

snapshot_hashes() {
  local -a files=(
    "$HOME/.bashrc"
    "$HOME/bashrc.postcustom"
    "$HOME/waveterm.rc"
    "$HOME/.bash_modules"
    "$HOME/.local/bin/sentinel-tty"
  )
  local f
  for f in "${files[@]}"; do
    if [[ -f "$f" ]]; then
      sha256sum "$f"
    else
      echo "MISSING  $f"
    fi
  done
}

echo "[test] install #1"
run_install

[[ -d "$HOME/bash_modules.d" ]] || fail "modules dir not created"
[[ -f "$HOME/bash_modules.d/fabric_integration.module" ]] || fail "fabric module not installed"

# Kitty integration must not activate on headless (no DISPLAY/WAYLAND_DISPLAY)
[[ ! -f "$HOME/.local/bin/sentinel-tty" ]] || fail "sentinel-tty should not be installed in headless/non-interactive"
[[ ! -f "$XDG_CONFIG_HOME/kitty/kitty.conf" ]] || fail "kitty.conf should not be written in headless mode"

snap1="$(snapshot_hashes)"
hash1="$(printf '%s\n' "$snap1" | sha256sum | awk '{print $1}')"

echo "[test] install #2 (idempotency)"
run_install

snap2="$(snapshot_hashes)"
hash2="$(printf '%s\n' "$snap2" | sha256sum | awk '{print $1}')"
if [[ "$hash1" != "$hash2" ]]; then
  echo "[debug] snapshot #1:"
  printf '%s\n' "$snap1"
  echo "[debug] snapshot #2:"
  printf '%s\n' "$snap2"
  echo "[debug] state file:"
  if [[ -f "$HOME/install.state" ]]; then
    cat "$HOME/install.state"
  else
    echo "MISSING $HOME/install.state"
  fi
  fail "install is not idempotent for stable outputs"
fi

# Ensure .bashrc only has one SENTINEL integration block (idempotent patching)
count="$(rg -n "# SENTINEL Framework Integration" "$HOME/.bashrc" | wc -l | tr -d ' ')"
[[ "$count" -le 1 ]] || fail ".bashrc integration duplicated ($count)"

# Telemetry emit must be a no-op when disabled and not block
start_ns="$(date +%s%N)"
# shellcheck source=/dev/null
source "${PROJECT_ROOT}/installer/helpers.sh"
FABRIC_ENABLED=0 FABRIC_TELEMETRY_ENABLED=0 installer_fabric_emit_event "test" "{}" || true
end_ns="$(date +%s%N)"
elapsed_ns=$((end_ns - start_ns))
[[ "$elapsed_ns" -lt 200000000 ]] || fail "telemetry no-op too slow (${elapsed_ns}ns)"

# Shellcheck on modified scripts (fast gate)
if command -v shellcheck >/dev/null 2>&1; then
  shellcheck -x \
    "${PROJECT_ROOT}/installer/lib/preflight.sh" \
    "${PROJECT_ROOT}/installer/lib/install_core.sh" \
    "${PROJECT_ROOT}/installer/helpers.sh" \
    "${PROJECT_ROOT}/installer/dependencies.sh" \
    "${PROJECT_ROOT}/installer/bash.sh" \
    "${PROJECT_ROOT}/bash_modules.d/fabric_integration.module" \
    "${PROJECT_ROOT}/bash_modules.d/hmac.module" \
    "${PROJECT_ROOT}/bash_modules.d/python_integration.module" >/dev/null
fi

python3 -m py_compile "${PROJECT_ROOT}/installer/config.py"

if command -v shfmt >/dev/null 2>&1; then
  shfmt -d \
    "${PROJECT_ROOT}/installer/lib/preflight.sh" \
    "${PROJECT_ROOT}/installer/lib/install_core.sh" \
    "${PROJECT_ROOT}/installer/helpers.sh" \
    "${PROJECT_ROOT}/installer/dependencies.sh" \
    "${PROJECT_ROOT}/installer/bash.sh" \
    "${PROJECT_ROOT}/bash_modules.d/fabric_integration.module" \
    "${PROJECT_ROOT}/bash_modules.d/hmac.module" \
    "${PROJECT_ROOT}/bash_modules.d/python_integration.module" >/dev/null
fi

echo "OK"
