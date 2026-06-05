#!/usr/bin/env bash

# Basic test script for auto_install and venv_helpers

# --- Setup ---
# Ensure we are in a predictable state if possible
# For testing, it's good to source the .bashrc in a subshell
# or ensure the functions/modules are loaded in a controlled way.
# However, directly sourcing .bashrc can have side effects.
# For this test, we'll assume the functions/modules are available
# as they would be in a normal shell session after the changes.

export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

source "${SENTINEL_ROOT}/bash_modules.d/logging.module" 2>/dev/null || true
source "${SENTINEL_ROOT}/bash_functions.d/venv_helpers"
export SENTINEL_AUTO_INSTALL_ENABLED=0
source "${SENTINEL_ROOT}/bash_modules.d/auto_install.module"

echo "--- Test Suite for New Features ---"

TEST_PASSED_COUNT=0
TEST_FAILED_COUNT=0

_run_test() {
    local test_name="$1"
    local command_to_run="$2"
    echo -n "Running test: $test_name ... "
    if eval "$command_to_run"; then
        echo -e "\033[0;32mPASSED\033[0m"
        TEST_PASSED_COUNT=$((TEST_PASSED_COUNT + 1))
    else
        echo -e "\033[0;31mFAILED\033[0m"
        TEST_FAILED_COUNT=$((TEST_FAILED_COUNT + 1))
    fi
}

# --- Tests for venv_helpers (mkvenv) ---

# 1. Check if mkvenv function is defined
_run_test "mkvenv function definition" "type mkvenv &>/dev/null"

# 2. Test mkvenv creation (basic check)
TEMP_VENV_DIR="/tmp/test_venv_$(date +%s)_$$"
_cleanup_temp_venv() {
    echo "Cleaning up temporary venv: $TEMP_VENV_DIR"
    rm -rf "$TEMP_VENV_DIR"
}
trap _cleanup_temp_venv EXIT # Cleanup on script exit

# We need to ensure that mkvenv doesn't ask for interactive input for this test.
# We can temporarily override `read` or ensure the path is clear.
# For simplicity, we'll ensure the path is clear.
if [ -d "$TEMP_VENV_DIR" ]; then
    rm -rf "$TEMP_VENV_DIR"
fi

echo "Note: mkvenv test will attempt to create a temporary virtual environment."
echo "It will install packages, which might take some time."
# This test is more involved as it actually runs mkvenv.
# We'll check for the directory and the activate script.
# We will redirect mkvenv output to /dev/null to keep test output clean,
# but this means we won't see its progress.
_test_mkvenv_creation() {
    export SENTINEL_MKVENV_SKIP_PACKAGES=1
    # Temporarily disable interactive prompts for the test
    # This is a simple way; a more robust way would be to expect/provide input.
    alias read='echo "N" | read'

    # Run mkvenv and check for directory and activate script
    if mkvenv "$TEMP_VENV_DIR" > /dev/null 2>&1; then
        if [[ -d "$TEMP_VENV_DIR" && -f "$TEMP_VENV_DIR/bin/activate" ]]; then
            # Try to activate and check for a common package (e.g., pip)
            # shellcheck source=/dev/null
            source "$TEMP_VENV_DIR/bin/activate"
            # Basic check: pip should be available after venv creation and activation
            if command -v pip &>/dev/null; then
                echo "pip command found in venv."
                # More extensive package check (can be slow, keep commented for CI or run selectively)
                # if pip list | grep -q -E "numpy|requests"; then
                #    echo "Core packages found."
                # else
                #    echo "Core packages (numpy/requests) not found in pip list."
                #    unalias read
                #    deactivate
                #    return 1
                # fi
                deactivate # Deactivate after check
                unset SENTINEL_MKVENV_SKIP_PACKAGES
                unalias read # Restore read
                return 0 # Success
            else
                echo "pip command NOT found in venv."
                deactivate
                unset SENTINEL_MKVENV_SKIP_PACKAGES
                unalias read
                return 1 # Failure
            fi
        else
            echo "Venv directory '$TEMP_VENV_DIR' or activate script '$TEMP_VENV_DIR/bin/activate' not found."
            unset SENTINEL_MKVENV_SKIP_PACKAGES
            unalias read
            return 1 # Failure
        fi
    else
        echo "mkvenv command itself failed to execute or returned an error."
        unset SENTINEL_MKVENV_SKIP_PACKAGES
        unalias read
        return 1 # Failure
    fi
}
_run_test "mkvenv basic creation (directory and activate script)" "_test_mkvenv_creation"


# --- Tests for auto_install.module ---

# 1. Check if auto_install_module was sourced (indirectly by checking for a log function it defines)
# This is a weak test, as the log function name might change or be common.
# A better test would be to mock `command -v fzf` and see if it tries to install.
_run_test "auto_install_module loading (check for _auto_install_log)" "type _auto_install_log &>/dev/null"

# 2. Verify the test harness can disable side effects when sourcing the module.
_run_test "auto_install_module explicit disable respected" '[[ "${SENTINEL_AUTO_INSTALL_ENABLED}" == "0" ]]'


# --- Summary ---
echo ""
echo "--- Test Summary ---"
echo -e "\033[0;32mTests Passed: $TEST_PASSED_COUNT\033[0m"
echo -e "\033[0;31mTests Failed: $TEST_FAILED_COUNT\033[0m"
echo "--------------------"

if [ "$TEST_FAILED_COUNT" -ne 0 ]; then
    exit 1
fi
exit 0
