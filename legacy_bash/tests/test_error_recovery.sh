#!/usr/bin/env bash
# SENTINEL Error Recovery and Graceful Degradation Test Suite
# Tests circuit breakers, fallbacks, and degradation modes

export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

# Source required modules
SENTINEL_MODULES_PATH="${SENTINEL_MODULES_PATH:-${SENTINEL_ROOT}/bash_modules.d}"

# Load core modules
source "${SENTINEL_MODULES_PATH}/logging.module" 2>/dev/null || {
    echo "Warning: Logging module not available, using stdout" >&2
}

source "${SENTINEL_MODULES_PATH}/error_recovery.module" || {
    echo "Error: Could not load error recovery module" >&2
    exit 1
}

source "${SENTINEL_MODULES_PATH}/fallback_registry.module" || {
    echo "Error: Could not load fallback registry module" >&2
    exit 1
}

# Test configuration
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Test reporting functions
test_start() {
    local test_name="$1"
    echo ""
    echo "=== Test: $test_name ==="
    ((TEST_COUNT++))
}

test_pass() {
    local message="${1:-Test passed}"
    echo "✓ $message"
    ((PASS_COUNT++))
}

test_fail() {
    local message="${1:-Test failed}"
    echo "✗ $message"
    ((FAIL_COUNT++))
}

# Test 1: Circuit Breaker Basic Functionality
test_start "Circuit Breaker Basic Functionality"

# Initialize circuit breaker
sentinel_circuit_breaker_init "test_component" 3 60

# Test initial state
if [[ "${SENTINEL_CIRCUIT_BREAKERS[test_component]}" == "closed" ]]; then
    test_pass "Circuit breaker initialized in closed state"
else
    test_fail "Circuit breaker not properly initialized"
fi

# Test failure tracking
sentinel_circuit_breaker_failure "test_component" "Test failure 1"
sentinel_circuit_breaker_failure "test_component" "Test failure 2"

if [[ "${SENTINEL_CIRCUIT_BREAKER_FAILURES[test_component]}" == "2" ]]; then
    test_pass "Failure count tracked correctly"
else
    test_fail "Failure count incorrect: ${SENTINEL_CIRCUIT_BREAKER_FAILURES[test_component]}"
fi

# Test circuit breaker opening
sentinel_circuit_breaker_failure "test_component" "Test failure 3"

if [[ "${SENTINEL_CIRCUIT_BREAKERS[test_component]}" == "open" ]]; then
    test_pass "Circuit breaker opened after threshold"
else
    test_fail "Circuit breaker failed to open"
fi

# Test 2: Circuit Breaker Command Execution
test_start "Circuit Breaker Command Execution"

# Define test functions
successful_function() {
    echo "Function executed successfully"
    return 0
}

failing_function() {
    echo "Function failed" >&2
    return 1
}

# Test successful execution
if sentinel_with_circuit_breaker "test_success" successful_function >/dev/null 2>&1; then
    test_pass "Successful function executed through circuit breaker"
else
    test_fail "Circuit breaker blocked successful function"
fi

# Test failing execution
sentinel_circuit_breaker_init "test_fail" 2 60
sentinel_with_circuit_breaker "test_fail" failing_function >/dev/null 2>&1
sentinel_with_circuit_breaker "test_fail" failing_function >/dev/null 2>&1

if [[ "${SENTINEL_CIRCUIT_BREAKERS[test_fail]}" == "open" ]]; then
    test_pass "Circuit breaker opened on repeated failures"
else
    test_fail "Circuit breaker failed to open on failures"
fi

# Test 3: Fallback Registration and Execution
test_start "Fallback Registration and Execution"

# Define fallback function
test_fallback() {
    echo "Fallback executed"
    return 0
}

# Register fallback
sentinel_register_fallback "test_fallback_component" "test_fallback"

if [[ "${SENTINEL_MODULE_FALLBACKS[test_fallback_component]}" == "test_fallback" ]]; then
    test_pass "Fallback registered successfully"
else
    test_fail "Fallback registration failed"
fi

# Trigger fallback
output=$(sentinel_trigger_fallback "test_fallback_component" 2>&1)
if [[ "$output" == *"Fallback executed"* ]]; then
    test_pass "Fallback executed successfully"
else
    test_fail "Fallback execution failed"
fi

# Test 4: Degradation Modes
test_start "Degradation Modes"

# Test mode switching
sentinel_set_degradation_mode "minimal"
if [[ "$SENTINEL_FALLBACK_MODE" == "minimal" ]]; then
    test_pass "Degradation mode set to minimal"
else
    test_fail "Failed to set degradation mode"
fi

# Test feature availability
if sentinel_feature_available "core_feature" "core"; then
    test_pass "Core features available in minimal mode"
else
    test_fail "Core features not available in minimal mode"
fi

if ! sentinel_feature_available "optional_feature" "optional"; then
    test_pass "Optional features correctly disabled in minimal mode"
else
    test_fail "Optional features incorrectly available in minimal mode"
fi

# Test 5: Error Context Capture
test_start "Error Context Capture"

# Capture error context
sentinel_capture_error_context "test_component" "test_phase"

# Check if context file was created
context_files=$(find "$SENTINEL_ERROR_RECOVERY_DIR" -name "test_component_test_phase_*.context" 2>/dev/null | wc -l)
if [[ $context_files -gt 0 ]]; then
    test_pass "Error context captured successfully"
else
    test_fail "Error context capture failed"
fi

# Test 6: Module Fallbacks
test_start "Module Fallbacks"

# Test FZF fallback
fzf_module_fallback >/dev/null 2>&1
if type select_file &>/dev/null; then
    test_pass "FZF fallback functions created"
else
    test_fail "FZF fallback failed"
fi

# Test ML fallback
sentinel_ml_fallback >/dev/null 2>&1
if type suggest_command &>/dev/null; then
    test_pass "ML fallback functions created"
else
    test_fail "ML fallback failed"
fi

# Test 7: Safe Module Loading
test_start "Safe Module Loading"

# Define a module fallback
nonexistent_module_fallback() {
    echo "Nonexistent module fallback activated"
    export _MODULE_NONEXISTENT_LOADED=1
}

# Try to load non-existent module with fallback
sentinel_safe_module_load "nonexistent_module" "nonexistent_module_fallback" >/dev/null 2>&1

if [[ -n "${_MODULE_NONEXISTENT_LOADED}" ]]; then
    test_pass "Safe module load triggered fallback"
else
    test_fail "Safe module load failed to trigger fallback"
fi

# Test 8: Error Recovery Status
test_start "Error Recovery Status"

# Generate status report
status_output=$(sentinel_error_recovery_status 2>&1)
if [[ "$status_output" == *"Circuit Breaker States:"* ]] && [[ "$status_output" == *"Registered Fallbacks:"* ]]; then
    test_pass "Error recovery status report generated"
else
    test_fail "Error recovery status report incomplete"
fi

# Test 9: Graceful Degradation in Safe Mode
test_start "Graceful Degradation in Safe Mode"

# Switch to safe mode
sentinel_set_degradation_mode "safe"

# Test that only core features are available
if sentinel_feature_available "logging" "core"; then
    test_pass "Core features available in safe mode"
else
    test_fail "Core features not available in safe mode"
fi

if ! sentinel_feature_available "ml_predict" "optional"; then
    test_pass "Optional features correctly disabled in safe mode"
else
    test_fail "Optional features incorrectly available in safe mode"
fi

# Test 10: Error Report Generation
test_start "Error Report Generation"

# Generate error report
report_file=$(sentinel_generate_error_report 2>&1 | tail -1)
if [[ -f "$report_file" ]]; then
    test_pass "Error report generated: $report_file"
    
    # Check report contents
    if grep -q "SENTINEL Error Report" "$report_file"; then
        test_pass "Error report contains expected content"
    else
        test_fail "Error report missing expected content"
    fi
else
    test_fail "Error report generation failed"
fi

# Test Summary
echo ""
echo "========================================"
echo "Test Summary:"
echo "  Total Tests: $TEST_COUNT"
echo "  Passed: $PASS_COUNT"
echo "  Failed: $FAIL_COUNT"
echo "  Success Rate: $(( PASS_COUNT * 100 / TEST_COUNT ))%"
echo "========================================"

# Cleanup
echo ""
echo "Cleaning up test artifacts..."
rm -f "$SENTINEL_ERROR_RECOVERY_DIR"/test_component_*.context 2>/dev/null
unset SENTINEL_CIRCUIT_BREAKERS[test_component]
unset SENTINEL_CIRCUIT_BREAKERS[test_success]
unset SENTINEL_CIRCUIT_BREAKERS[test_fail]
unset _MODULE_NONEXISTENT_LOADED

# Reset to graceful mode
sentinel_set_degradation_mode "graceful"

# Exit with appropriate code
if [[ $FAIL_COUNT -eq 0 ]]; then
    echo ""
    echo "✓ All tests passed!"
    exit 0
else
    echo ""
    echo "✗ Some tests failed. Please review the output above."
    exit 1
fi