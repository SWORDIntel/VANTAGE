# VANTAGE Testing Guide

## Overview

Testing is crucial for VANTAGE's reliability. This guide covers testing strategies, tools, and best practices for ensuring your changes work correctly.

## Testing Levels

### 1. Syntax Testing
Quick validation that bash scripts are syntactically correct.

```bash
# Test single file
bash -n bashrc

# Test all bash files
find . -name "*.sh" -o -name "*.module" | while read -r file; do
    echo "Checking $file..."
    bash -n "$file" || echo "SYNTAX ERROR in $file"
done
```

### 2. Unit Testing
Test individual functions and modules in isolation.

```bash
# Example unit test
cat > tests/unit/test_module_functions.sh << 'EOF'
#!/bin/bash
# Unit tests for module functions

# Test setup
source bash_modules

# Test: is_module_loaded function
test_is_module_loaded() {
    # Setup
    LOADED_MODULES=("test_module")
    
    # Test positive case
    if is_module_loaded "test_module"; then
        echo "PASS: is_module_loaded detects loaded module"
    else
        echo "FAIL: is_module_loaded failed to detect loaded module"
        return 1
    fi
    
    # Test negative case
    if ! is_module_loaded "nonexistent_module"; then
        echo "PASS: is_module_loaded correctly reports missing module"
    else
        echo "FAIL: is_module_loaded false positive"
        return 1
    fi
}

# Run tests
test_is_module_loaded
EOF
```

### 3. Integration Testing
Test how components work together.

```bash
# Integration test example
cat > tests/integration/test_module_loading.sh << 'EOF'
#!/bin/bash
# Test complete module loading process

# Start with clean environment
unset LOADED_MODULES
source bash_modules

# Test loading a module with dependencies
load_module "autocomplete"

# Verify module and dependencies loaded
if is_module_loaded "autocomplete" && is_module_loaded "config_cache"; then
    echo "PASS: Module and dependencies loaded"
else
    echo "FAIL: Module loading failed"
    exit 1
fi
EOF
```

### 4. System Testing
Test the complete VANTAGE system.

```bash
# The main system test
./scripts/vantage_postinstall_check.sh
```

## Testing Tools

### 1. Built-in Test Runner

```bash
# VANTAGE test runner
cat > tests/run_tests.sh << 'EOF'
#!/bin/bash
# VANTAGE Test Runner

TESTS_DIR="$(dirname "$0")"
PASSED=0
FAILED=0

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Run a test file
run_test() {
    local test_file="$1"
    echo -n "Running $(basename "$test_file")... "
    
    if bash "$test_file" > /tmp/test_output 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        echo "Output:"
        cat /tmp/test_output
        ((FAILED++))
    fi
}

# Find and run all tests
find "$TESTS_DIR" -name "test_*.sh" -type f | while read -r test; do
    run_test "$test"
done

# Summary
echo "========================"
echo "Test Results:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "========================"

[[ $FAILED -eq 0 ]] && exit 0 || exit 1
EOF

chmod +x tests/run_tests.sh
```

### 2. Module Test Framework

```bash
# Framework for testing modules
cat > tests/module_test_framework.sh << 'EOF'
#!/bin/bash
# Module testing framework

# Test a module in isolation
test_module() {
    local module_name="$1"
    local module_file="bash_modules.d/${module_name}.module"
    
    # Check file exists
    [[ -f "$module_file" ]] || {
        echo "Module file not found: $module_file"
        return 1
    }
    
    # Test in subshell to avoid pollution
    (
        # Minimal environment
        source bash_modules
        
        # Load the module
        load_module "$module_name" || exit 1
        
        # Run module self-test if available
        if type "_test_${module_name}" &>/dev/null; then
            "_test_${module_name}"
        else
            echo "No self-test for module: $module_name"
        fi
    )
}

# Test all modules
test_all_modules() {
    for module_file in bash_modules.d/*.module; do
        module_name=$(basename "$module_file" .module)
        echo "Testing module: $module_name"
        test_module "$module_name"
        echo "---"
    done
}
EOF
```

### 3. Performance Testing

```bash
# Performance test framework
cat > tests/performance/measure_startup.sh << 'EOF'
#!/bin/bash
# Measure bash startup time with VANTAGE

# Baseline (no VANTAGE)
echo "Measuring baseline bash startup..."
baseline=$(( \
    time bash -c 'exit' \
) 2>&1 | grep real | awk '{print $2}')

# With VANTAGE
echo "Measuring VANTAGE bash startup..."
vantage=$(( \
    time bash --rcfile bashrc -c 'exit' \
) 2>&1 | grep real | awk '{print $2}')

echo "Results:"
echo "  Baseline: $baseline"
echo "  VANTAGE: $vantage"

# Module load times
echo -e "\nModule load times:"
MODULE_TIMING=1 bash --rcfile bashrc -c 'exit' 2>&1 | grep "Module.*loaded in"
EOF
```

## Test Scenarios

### 1. Module Testing Checklist

```bash
# For each module, test:
- [ ] Module loads without errors
- [ ] Dependencies are satisfied
- [ ] Functions work as expected
- [ ] Cleanup removes all traces
- [ ] Performance is acceptable
- [ ] Error handling works
- [ ] Configuration is loaded correctly
- [ ] Help text is available
```

### 2. Security Testing

```bash
# Security test scenarios
cat > tests/security/test_module_security.sh << 'EOF'
#!/bin/bash
# Security tests for modules

test_hmac_verification() {
    # Enable HMAC verification
    ENABLE_HMAC_VERIFICATION=true
    
    # Test with valid module
    if load_module "logging"; then
        echo "PASS: Valid module loaded with HMAC"
    else
        echo "FAIL: Valid module rejected"
        return 1
    fi
    
    # Test with tampered module
    cp bash_modules.d/logging.module /tmp/
    echo "# Tampered" >> /tmp/logging.module
    
    if ! load_module_from_file "/tmp/logging.module"; then
        echo "PASS: Tampered module rejected"
    else
        echo "FAIL: Tampered module accepted"
        return 1
    fi
}

test_permission_checks() {
    # Test world-writable module rejection
    cp bash_modules.d/logging.module /tmp/unsafe.module
    chmod 666 /tmp/unsafe.module
    
    if ! load_module_from_file "/tmp/unsafe.module"; then
        echo "PASS: Unsafe permissions rejected"
    else
        echo "FAIL: Unsafe module accepted"
        return 1
    fi
}
EOF
```

### 3. Compatibility Testing

```bash
# Test on different bash versions
cat > tests/compatibility/test_bash_versions.sh << 'EOF'
#!/bin/bash
# Test compatibility with different bash versions

test_bash_version() {
    local version="$1"
    echo "Testing with bash $version..."
    
    # Check if version available
    if command -v "bash$version" &>/dev/null; then
        "bash$version" --rcfile bashrc -c 'echo "VANTAGE loaded successfully"'
    else
        echo "Bash $version not available, skipping"
    fi
}

# Test common versions
for version in 4.0 4.2 4.3 4.4 5.0 5.1; do
    test_bash_version "$version"
done
EOF
```

## Testing Best Practices

### 1. Test File Organization

```
tests/
├── unit/              # Unit tests for individual functions
├── integration/       # Integration tests
├── performance/       # Performance benchmarks
├── security/          # Security tests
├── compatibility/     # Compatibility tests
├── fixtures/          # Test data and fixtures
└── run_tests.sh       # Main test runner
```

### 2. Writing Good Tests

```bash
# Good test example
test_feature_with_edge_cases() {
    local test_name="feature_edge_cases"
    
    # Setup
    setup_test_environment
    
    # Test normal case
    if feature_function "normal input"; then
        log_test_pass "$test_name" "normal case"
    else
        log_test_fail "$test_name" "normal case failed"
    fi
    
    # Test edge cases
    # Empty input
    if ! feature_function ""; then
        log_test_pass "$test_name" "empty input rejected"
    else
        log_test_fail "$test_name" "empty input accepted"
    fi
    
    # Special characters
    if feature_function "test\$pecial"; then
        log_test_pass "$test_name" "special chars handled"
    else
        log_test_fail "$test_name" "special chars failed"
    fi
    
    # Cleanup
    cleanup_test_environment
}
```

### 3. Test Utilities

```bash
# Common test utilities
cat > tests/test_utils.sh << 'EOF'
#!/bin/bash
# Common utilities for tests

# Test result tracking
declare -g TESTS_PASSED=0
declare -g TESTS_FAILED=0

# Logging functions
log_test_pass() {
    local test_name="$1"
    local message="$2"
    echo "[PASS] $test_name: $message"
    ((TESTS_PASSED++))
}

log_test_fail() {
    local test_name="$1"
    local message="$2"
    echo "[FAIL] $test_name: $message" >&2
    ((TESTS_FAILED++))
}

# Assertions
assert_equals() {
    local expected="$1"
    local actual="$2"
    local message="${3:-Values should be equal}"
    
    if [[ "$expected" == "$actual" ]]; then
        return 0
    else
        echo "Assertion failed: $message"
        echo "  Expected: $expected"
        echo "  Actual: $actual"
        return 1
    fi
}

assert_command_succeeds() {
    local command="$1"
    local message="${2:-Command should succeed}"
    
    if eval "$command" &>/dev/null; then
        return 0
    else
        echo "Assertion failed: $message"
        echo "  Command: $command"
        return 1
    fi
}

# Test environment
create_test_environment() {
    export TEST_DIR=$(mktemp -d)
    export OLD_PWD="$PWD"
    cd "$TEST_DIR"
}

cleanup_test_environment() {
    cd "$OLD_PWD"
    rm -rf "$TEST_DIR"
}

# Summary
print_test_summary() {
    echo "========================"
    echo "Test Summary:"
    echo "  Passed: $TESTS_PASSED"
    echo "  Failed: $TESTS_FAILED"
    echo "========================"
    
    [[ $TESTS_FAILED -eq 0 ]]
}
EOF
```

## Continuous Testing

### 1. Pre-commit Hook

```bash
# .git/hooks/pre-commit
#!/bin/bash
# Run tests before committing

echo "Running pre-commit tests..."

# Syntax check
find . -name "*.sh" -o -name "*.module" | while read -r file; do
    bash -n "$file" || exit 1
done

# Run quick tests
./tests/run_tests.sh --quick || exit 1

echo "All tests passed!"
```

### 2. Watch Mode

```bash
# Watch for changes and run tests
cat > tests/watch_tests.sh << 'EOF'
#!/bin/bash
# Watch mode for continuous testing

watch_and_test() {
    echo "Watching for changes... (Ctrl+C to stop)"
    
    while true; do
        # Use inotifywait if available
        if command -v inotifywait &>/dev/null; then
            inotifywait -r -e modify,create bash_modules.d/ tests/
        else
            sleep 2
        fi
        
        clear
        echo "Running tests..."
        ./tests/run_tests.sh
    done
}

watch_and_test
EOF
```

## Debugging Tests

### 1. Enable Debug Output

```bash
# Debug mode for tests
DEBUG=1 ./tests/run_tests.sh

# Or in specific test
set -x  # Enable trace
test_function
set +x  # Disable trace
```

### 2. Interactive Debugging

```bash
# Add breakpoint in test
test_complex_feature() {
    setup_test
    
    # Breakpoint
    echo "Breakpoint: Press Enter to continue..."
    read -r
    
    # Continue test
    run_feature
}
```

### 3. Test Isolation

```bash
# Run test in isolated environment
cat > tests/isolated_test.sh << 'EOF'
#!/bin/bash
# Run test in isolation

run_isolated_test() {
    local test_script="$1"
    
    # Create clean environment
    env -i \
        HOME="$HOME" \
        PATH="/usr/bin:/bin" \
        bash "$test_script"
}

run_isolated_test "tests/unit/test_specific.sh"
EOF
```

## Test Documentation

### 1. Test Plan Template

```markdown
# Test Plan: [Feature Name]

## Overview
Brief description of what is being tested.

## Test Cases

### TC001: [Test Case Name]
**Objective**: What this test verifies
**Preconditions**: Required setup
**Steps**:
1. Step 1
2. Step 2
3. Step 3
**Expected Result**: What should happen
**Actual Result**: [PASS/FAIL]

### TC002: [Test Case Name]
...
```

### 2. Test Report Template

```markdown
# Test Report: VANTAGE v[VERSION]

## Summary
- **Date**: YYYY-MM-DD
- **Tester**: Name
- **Environment**: OS, Bash version
- **Overall Result**: PASS/FAIL

## Test Results

| Test Suite | Passed | Failed | Skipped |
|------------|--------|--------|---------|
| Unit       | 45     | 0      | 2       |
| Integration| 12     | 1      | 0       |
| Security   | 8      | 0      | 0       |
| Performance| 5      | 0      | 1       |

## Issues Found
1. Issue description and impact
2. ...

## Recommendations
- Recommendation 1
- Recommendation 2
```

Remember: Good tests are the foundation of reliable software. Test early, test often!