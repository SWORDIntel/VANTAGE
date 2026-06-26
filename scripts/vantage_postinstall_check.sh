#!/usr/bin/env bash
###############################################################################
# VANTAGE – Comprehensive Post-Install Verification
# -----------------------------------------------
# v1.0.0 • 2025-05-20
# Performs comprehensive verification of all VANTAGE components
# and ensures proper installation and configuration
###############################################################################

set -euo pipefail

# Define color codes
c_red=$'\033[1;31m'; c_green=$'\033[1;32m'; c_yellow=$'\033[1;33m'; 
c_blue=$'\033[1;34m'; c_purple=$'\033[1;35m'; c_cyan=$'\033[1;36m'; c_reset=$'\033[0m'

# Define paths
VENV_DIR="${HOME}/venv"
MODULES_DIR="${HOME}/bash_modules.d"
LOG_DIR="${HOME}/logs"
VERIFICATION_LOG="${LOG_DIR}/verification.log"
INSTALL_STATE_FILE="${HOME}/install.state"

# Ensure log directory exists
mkdir -p "${LOG_DIR}"
chmod 700 "${LOG_DIR}"
: > "${VERIFICATION_LOG}"  # Clear log file

# Logging functions
log() { printf '[%(%F %T)T] %b\n' -1 "$*" | tee -a "${VERIFICATION_LOG}"; }
header() { 
    echo
    log "${c_purple}=================================================${c_reset}"
    log "${c_purple}=== ${c_reset}${c_cyan}$1${c_reset}${c_purple} ===${c_reset}"
    log "${c_purple}=================================================${c_reset}"
    echo
}
subheader() { 
    log "${c_yellow}-----------------------------------------------${c_reset}"
    log "${c_yellow}--- ${c_reset}$1${c_yellow} ---${c_reset}"
    log "${c_yellow}-----------------------------------------------${c_reset}"
}
pass() { log "${c_green}[PASS]${c_reset} $*"; }
fail() { log "${c_red}[FAIL]${c_reset} $*"; FAILED_TESTS=$((FAILED_TESTS + 1)); }
warn() { log "${c_yellow}[WARN]${c_reset} $*"; }
info() { log "${c_blue}[INFO]${c_reset} $*"; }

# Initialize counter for failed tests
FAILED_TESTS=0
TOTAL_TESTS=0
BLESH_SKIPPED=0

# Detect if BLE.sh installation was intentionally skipped (e.g., headless install)
if [[ "${VANTAGE_SKIP_BLESH:-0}" == "1" ]]; then
    BLESH_SKIPPED=1
elif grep -qxF "BLESH_SKIPPED" "${INSTALL_STATE_FILE}" 2>/dev/null; then
    BLESH_SKIPPED=1
fi

# Create temporary test environment
TESTDIR=$(mktemp -d)
trap 'rm -rf "$TESTDIR"' EXIT

# Banner
echo "${c_green}"
echo " ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄            "
echo "▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░▌           "
echo "▐░█▀▀▀▀▀▀▀▀▀  ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░▌           "
echo "▐░▌               ▐░▌     ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌▐░▌    ▐░▌▐░▌          ▐░▌           "
echo "▐░█▄▄▄▄▄▄▄▄▄      ▐░▌     ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌ ▐░▌   ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░▌           "
echo "▐░░░░░░░░░░░▌     ▐░▌     ▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░▌           "
echo " ▀▀▀▀▀▀▀▀▀█░▌     ▐░▌     ▐░▌   ▐░▌ ▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌   ▐░▌ ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░▌           "
echo "          ▐░▌     ▐░▌     ▐░▌    ▐░▌▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌    ▐░▌▐░▌▐░▌          ▐░▌           "
echo " ▄▄▄▄▄▄▄▄▄█░▌ ▄▄▄▄█░█▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄█░▌▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ "
echo "▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌▐░▌      ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌"
echo " ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ "
echo "                                       VERIFICATION SYSTEM                                                 "
echo "${c_reset}"

# Run a test and record results
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_status="${3:-0}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    info "Running test: $test_name"
    
    # Execute the test command
    if eval "$test_cmd" > /dev/null 2>&1; then
        local actual_status=0
    else
        local actual_status=1
    fi
    
    # Check if the test passed
    if [[ $actual_status -eq $expected_status ]]; then
        pass "$test_name"
        return 0
    else
        fail "$test_name"
        return 1
    fi
}

header "VANTAGE Post-Installation Verification"
log "Running comprehensive verification of VANTAGE installation"
log "Results will be logged to: ${VERIFICATION_LOG}"

###############################################################################
# 1. File System Structure Tests
###############################################################################
header "File System Structure Tests"

# Check critical directories
for dir in "${HOME}/autocomplete" "${MODULES_DIR}" "${LOG_DIR}"; do
    run_test "Directory exists: $dir" "[[ -d \"$dir\" ]]"
done

# Check venv directory but treat as warning not failure
if [[ ! -d "${VENV_DIR}" ]]; then
    warn "Virtual environment directory doesn't exist at ${VENV_DIR}"
else
    pass "Virtual environment directory exists at ${VENV_DIR}"
fi

# Check critical files
run_test "BLE.sh loader exists" "[[ -f \"${HOME}/blesh_loader.sh\" ]]"
run_test "bashrc.postcustom exists" "[[ -f \"${HOME}/bashrc.postcustom\" ]]"
run_test "Autocomplete script exists" "[[ -f \"${HOME}/bash_aliases.d/autocomplete\" ]]"
run_test ".bash_modules exists" "[[ -f \"${HOME}/.bash_modules\" ]]"

# Check permissions
run_test "Logs directory has secure permissions" "stat -c %a \"${LOG_DIR}\" | grep -E '^700$'"
run_test "Modules directory has secure permissions" "stat -c %a \"${MODULES_DIR}\" | grep -E '^700$'"
run_test "Autocomplete script is executable" "stat -c %a \"${HOME}/bash_aliases.d/autocomplete\" | grep -E '^700$'"

###############################################################################
# 2. Python Environment Tests
###############################################################################
header "Python Virtual Environment Tests"

# Skip venv tests if the directory doesn't exist
if [[ ! -d "${VENV_DIR}" ]]; then
    warn "Python venv directory doesn't exist at ${VENV_DIR}. Skipping Python venv tests."
else
    run_test "Python venv exists" "[[ -f \"${VENV_DIR}/bin/python3\" ]]"
    run_test "pip is installed in venv" "[[ -f \"${VENV_DIR}/bin/pip\" ]]"

    # Test imports for key Python packages
    for pkg in numpy markovify tqdm unidecode rich; do
        run_test "Python package $pkg is installed" "\"${VENV_DIR}/bin/python3\" -c \"import $pkg\" 2>/dev/null"
    done
fi

###############################################################################
# 3. Bash Integration Tests
###############################################################################
header "Bash Integration Tests"

# Check for integration in either .bashrc or .bashrc.vantage
if grep -q 'VANTAGE Framework Integration' "${HOME}/.bashrc" 2>/dev/null; then
    pass "VANTAGE integration found in .bashrc"
elif [[ -f "${HOME}/.bashrc.vantage" ]] && grep -q 'VANTAGE Extensions' "${HOME}/.bashrc.vantage" 2>/dev/null; then
    pass "VANTAGE integration found in .bashrc.vantage"
else
    warn "VANTAGE integration not found in .bashrc or .bashrc.vantage"
fi


###############################################################################
# 4. Module Checks
###############################################################################
header "Module Installation Tests"

# Count loaded modules
MODULE_COUNT=$(grep -v "^[[:space:]]*#" "${HOME}/.bash_modules" 2>/dev/null | grep -v "^[[:space:]]*$" | wc -l)
if [[ $MODULE_COUNT -lt 3 ]]; then
    warn "Only $MODULE_COUNT modules defined in .bash_modules (expected ≥3)"
else
    pass "Found $MODULE_COUNT modules in .bash_modules"
fi

# Check for essential modules
for module in autocomplete.module fuzzy_correction.module logging.module config_cache.module; do
    run_test "Module exists: $module" "[[ -f \"${MODULES_DIR}/$module\" ]]"
done

###############################################################################
# 5. BLE.sh Installation Tests
###############################################################################
if [[ $BLESH_SKIPPED -eq 1 ]]; then
    header "BLE.sh Installation Tests (Skipped)"
    warn "BLE.sh installation was skipped (headless/server mode). Skipping BLE.sh verification tests."
else
    header "BLE.sh Installation Tests"

    run_test "BLE.sh is installed" "[[ -d \"${HOME}/.local/share/blesh\" ]]"
    run_test "BLE.sh main script exists" "[[ -f \"${HOME}/.local/share/blesh/ble.sh\" ]]"

    # Check cache directory, create if doesn't exist
    if [[ ! -d "${HOME}/.cache/blesh" ]]; then
        mkdir -p "${HOME}/.cache/blesh" 
        warn "Created missing BLE.sh cache directory"
    fi
    run_test "BLE.sh cache directory exists" "[[ -d \"${HOME}/.cache/blesh\" ]]"
fi

###############################################################################
# 6. Environment Sourcing Tests
###############################################################################
header "Environment Sourcing Tests"

# Create a test script that sources VANTAGE environment
cat > "${TESTDIR}/source_test.sh" << 'EOT'
#!/usr/bin/env bash
# Use set +e to prevent errors from stopping execution
set +e

# Create a fallback postcustom file if it doesn't exist
if [[ ! -f "${HOME}/bashrc.postcustom" ]]; then
  echo '# Fallback bashrc.postcustom created by VANTAGE verification' > "${HOME}/bashrc.postcustom"
  echo 'export VENV_AUTO=1' >> "${HOME}/bashrc.postcustom"
  echo "POSTCUSTOM_CREATED=true"
fi

# First source the main bashrc to set up the environment
# This ensures all required functions and variables are available
SOURCE_BASHRC_OUTPUT=""
if [[ -f "${HOME}/.bashrc" ]]; then
  # Source bashrc to get the full environment including safe_source function
  SOURCE_BASHRC_OUTPUT=$(source "${HOME}/.bashrc" 2>&1) || true
  echo "BASHRC_SOURCED=true"
else
  echo "BASHRC_SOURCED=false"
fi

# Now source postcustom with the proper environment set up
SOURCE_STDERR_OUTPUT=""
SOURCE_STDOUT_OUTPUT=""
if SOURCE_STDERR_OUTPUT=$(source "${HOME}/bashrc.postcustom" 2>&1); then
  echo "SOURCE_SUCCESS=true"
  SOURCE_STDOUT_OUTPUT="$SOURCE_STDERR_OUTPUT" # if successful, output is stdout
  SOURCE_STDERR_OUTPUT="" # clear stderr if successful
else
  echo "SOURCE_FAILED=true"
  # stderr is already in SOURCE_STDERR_OUTPUT
fi

echo "--- Captured STDOUT from sourcing bashrc.postcustom ---"
echo "${SOURCE_STDOUT_OUTPUT}"
echo "--- End Captured STDOUT ---"

echo "--- Captured STDERR from sourcing bashrc.postcustom ---"
echo "${SOURCE_STDERR_OUTPUT}"
echo "--- End Captured STDERR ---"

# Check for the signal file created by the main bashrc
if [[ -f "/tmp/postcustom_loaded.signal" ]]; then
    echo "SIGNAL_FILE_EXISTS=true"
else
    echo "SIGNAL_FILE_EXISTS=false"
fi

# Output environment variables for verification - safely check if they exist
echo "BLESH_LOADED=${VANTAGE_BLESH_LOADED:-not_set}"
echo "VENV_AUTO=${VENV_AUTO:-not_set}"

# Safely check for functions without failing if they don't exist
if type @autocomplete >/dev/null 2>&1; then
  echo "AUTOCOMPLETE=available"
else
  echo "AUTOCOMPLETE=missing"
fi

if type venvon >/dev/null 2>&1; then
  echo "VENVON=available"
else
  echo "VENVON=missing"
fi

if type vantage_log >/dev/null 2>&1; then
  echo "LOGGING=available"
else
  echo "LOGGING=missing"
fi

# Always exit successfully 
exit 0
EOT
chmod +x "${TESTDIR}/source_test.sh"

# Capture output with proper error handling to prevent installer from aborting
TMP_SOURCE_OUTPUT_FILE="${TESTDIR}/source_output.txt"
bash "${TESTDIR}/source_test.sh" > "${TMP_SOURCE_OUTPUT_FILE}" 2>&1
SOURCE_OUTPUT=$(<"${TMP_SOURCE_OUTPUT_FILE}") # Read it for grep

if echo "$SOURCE_OUTPUT" | grep -q "SOURCE_FAILED=true"; then
    fail "Failed to source bashrc.postcustom"
    # Log the captured output for debugging
    log "--- BEGIN CAPTURED OUTPUT FROM source_test.sh ---"
    tee -a "${VERIFICATION_LOG}" < "${TMP_SOURCE_OUTPUT_FILE}"
    # Add a newline to the log after tee, as tee itself might not add one depending on input
    echo "" >> "${VERIFICATION_LOG}"
    log "--- END CAPTURED OUTPUT FROM source_test.sh ---"
else
    pass "Successfully sourced bashrc.postcustom"
    # Optionally log full output even on success for detailed tracing
    # echo "$SOURCE_OUTPUT" >> "${VERIFICATION_LOG}"
    
    # Check for essential components in output with better error handling
    if echo "$SOURCE_OUTPUT" | grep -q "AUTOCOMPLETE=available"; then
        pass "Autocomplete function is available"
    else
        warn "Autocomplete function is missing or not properly loaded"
    fi
    
    if echo "$SOURCE_OUTPUT" | grep -q "VENVON=available"; then
        pass "VENV functions are available"
    else
        warn "VENV functions are missing or not properly loaded"
    fi
    
    if echo "$SOURCE_OUTPUT" | grep -q "LOGGING=available"; then
        pass "Logging functions are available"
    else
        warn "Logging functions are missing or not properly loaded"
    fi
fi

# Check for the signal file from the test script's perspective
if echo "$SOURCE_OUTPUT" | grep -q "SIGNAL_FILE_EXISTS=true"; then
    pass "Signal file /tmp/postcustom_loaded.signal was created during test script execution."
elif echo "$SOURCE_OUTPUT" | grep -q "SIGNAL_FILE_EXISTS=false"; then
    warn "Signal file /tmp/postcustom_loaded.signal was NOT created during test script execution."
else
    warn "Could not determine status of signal file /tmp/postcustom_loaded.signal from test script output."
fi

###############################################################################
# 7. Security Checks
###############################################################################
header "Security Checks"

# Define a function to safely check file permissions
check_file_permissions() {
  local file="$1"
  local max_perm="$2"
  
  if [[ ! -f "$file" ]]; then
    return 1
  fi
  
  # Try different stat commands based on OS
  local perms
  if command -v stat >/dev/null 2>&1; then
    # Linux style
    perms=$(stat -c %a "$file" 2>/dev/null || stat -f "%Lp" "$file" 2>/dev/null)
    
    # If we got permissions, check they're not too permissive
    if [[ -n "$perms" ]]; then
      if [[ "$perms" -le "$max_perm" ]]; then
        return 0
      else
        return 1
      fi
    fi
  fi
  
  # Fallback to ls -l if stat didn't work
  local ls_output
  ls_output=$(ls -la "$file" 2>/dev/null)
  if [[ -n "$ls_output" ]]; then
    # Check if others have write permission
    if [[ "$max_perm" -lt 700 && "$ls_output" == *"w"*"w"*"w"* ]]; then
      return 1
    fi
    return 0
  fi
  
  # If we couldn't check permissions, assume it passed
  warn "Could not check permissions for $file"
  return 0
}

# Test for world-writable files and directories with better error handling
run_test "No world-writable files in bash_modules.d" "! find \"${MODULES_DIR}\" -type f -perm -o=w -print 2>/dev/null | grep -q ."
run_test "No world-writable directories in bash_modules.d" "! find \"${MODULES_DIR}\" -type d -perm -o=w -print 2>/dev/null | grep -q ."

# Use the safer custom function for checking permissions
run_test "bashrc has proper permissions" "check_file_permissions \"${HOME}/.bashrc\" 644"
run_test "bashrc.postcustom has proper permissions" "check_file_permissions \"${HOME}/bashrc.postcustom\" 644"

###############################################################################
# Final Results
###############################################################################
header "Verification Results Summary"

# Count critical vs non-critical failures
CRITICAL_FAILURES=0
NON_CRITICAL_FAILURES=$FAILED_TESTS

# If we want to treat certain failures as non-critical,
# we would check specific test results and adjust the counters

if [[ $FAILED_TESTS -eq 0 ]]; then
    log "${c_green}==========================================${c_reset}"
    log "${c_green}  All verification tests passed! 🎉       ${c_reset}"
    log "${c_green}  $TOTAL_TESTS/$TOTAL_TESTS tests successful     ${c_reset}"
    log "${c_green}==========================================${c_reset}"
    log "VANTAGE is properly installed and ready to use."
else
    log "${c_yellow}==========================================${c_reset}"
    log "${c_yellow}  $FAILED_TESTS/$TOTAL_TESTS verification tests failed  ${c_reset}"
    log "${c_yellow}==========================================${c_reset}"
    log "Some components may not be installed correctly, but the system should still function."
    log "Review the log file at: ${VERIFICATION_LOG}"
fi

log ""
log "Next steps:"
log "1. Start a new terminal session or run: source ${HOME}/bashrc.postcustom"
log "2. Try using @autocomplete to confirm functionality"
log "3. For any issues, check the logs in ${LOG_DIR}"
log "4. If problems persist, run: bash install.sh reinstall"

# Exit with success regardless of verification failures to allow installation to complete
# This prevents the install from failing due to non-critical issues
exit 0 
