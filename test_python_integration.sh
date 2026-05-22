#!/usr/bin/env bash
# Test script for Python/Bash integration

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== SENTINEL Python/Bash Integration Test ==="
echo

FAILURES=0

record_failure() {
    FAILURES=$((FAILURES + 1))
}

# Check if we're in SENTINEL directory
if [[ ! -f "bash_modules.d/python_integration.module" ]]; then
    echo -e "${RED}Error: Must run from SENTINEL root directory${NC}"
    exit 1
fi

# Source the integration module
echo "Loading python_integration.module..."
source bash_modules.d/python_integration.module

if ! declare -f sentinel_state_get &>/dev/null; then
    echo -e "${RED}Failed to load python_integration module${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Module loaded successfully${NC}"
echo

# Test state management
echo "Testing State Management..."
sentinel_state_set "test_key" "test_value"
result=$(sentinel_state_get "test_key")
if [[ "$result" == "test_value" ]]; then
    echo -e "${GREEN}✓ State management working${NC}"
else
    echo -e "${RED}✗ State management failed${NC}"
    record_failure
fi

# Test configuration
echo
echo "Testing Configuration System..."
sentinel_config_set "test.setting" "enabled"
result=$(sentinel_config_get "test.setting")
if [[ "$result" == "enabled" ]]; then
    echo -e "${GREEN}✓ Configuration system working${NC}"
else
    echo -e "${RED}✗ Configuration system failed${NC}"
    record_failure
fi

# Test Python execution
echo
echo "Testing Python Execution..."
sentinel_python_exec -c "print('Hello from Python')" > /tmp/pytest.out 2>&1
if grep -q "Hello from Python" /tmp/pytest.out; then
    echo -e "${GREEN}✓ Python execution working${NC}"
else
    echo -e "${RED}✗ Python execution failed${NC}"
    cat /tmp/pytest.out
    record_failure
fi

# Test IPC
echo
echo "Testing IPC Channels..."
sentinel_ipc_create_channel "test_channel"
if [[ -p "${SENTINEL_IPC_DIR}/test_channel.in" ]]; then
    echo -e "${GREEN}✓ IPC channel created${NC}"
else
    echo -e "${RED}✗ IPC channel creation failed${NC}"
    record_failure
fi

# Test ML state sync (if available)
echo
echo "Testing ML State Sync..."
if [[ -f "bash_modules.d/ml_state_sync.module" ]]; then
    if source bash_modules.d/ml_state_sync.module && ml_sync_all; then
        echo -e "${GREEN}✓ ML state sync completed${NC}"
    else
        echo -e "${RED}✗ ML state sync failed${NC}"
        record_failure
    fi
else
    echo -e "${YELLOW}⚠ ML state sync module not found (optional)${NC}"
fi

# Run Python integration test
echo
echo "Running Python Integration Test Suite..."
if [[ -f "contrib/sentinel_integration_test.py" ]]; then
    chmod +x contrib/sentinel_integration_test.py
    if ! python3 contrib/sentinel_integration_test.py; then
        record_failure
    fi
else
    echo -e "${YELLOW}⚠ Python test suite not found${NC}"
fi

# Summary
echo
echo "=== Test Summary ==="
echo "State Directory: ${SENTINEL_STATE_DIR}"
echo "Config Directory: ${SENTINEL_CONFIG_DIR}"
echo "IPC Directory: ${SENTINEL_IPC_DIR}"
echo "Log Directory: ${SENTINEL_LOG_DIR}"
echo
if [[ $FAILURES -eq 0 ]]; then
    echo -e "${GREEN}Integration test completed!${NC}"
else
    echo -e "${RED}Integration test failed with ${FAILURES} issue(s).${NC}"
fi

# Cleanup
rm -f /tmp/pytest.out
sentinel_state_delete "test_key"
exit "$FAILURES"
