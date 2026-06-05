#!/usr/bin/env bash
# Test script for external tools integration

# Source test framework
source "$(dirname "$0")/test_framework.sh" 2>/dev/null || {
    # Simple test framework
    TESTS_PASSED=0
    TESTS_FAILED=0
    
    test_start() {
        echo "=== Testing External Tools Integration ==="
        echo
    }
    
    test_case() {
        echo -n "Testing $1... "
    }
    
    test_pass() {
        echo -e "\033[32mPASSED\033[0m"
        ((TESTS_PASSED++))
    }
    
    test_fail() {
        echo -e "\033[31mFAILED\033[0m: $1"
        ((TESTS_FAILED++))
    }
    
    test_summary() {
        echo
        echo "=== Test Summary ==="
        echo "Passed: $TESTS_PASSED"
        echo "Failed: $TESTS_FAILED"
        echo
        [[ $TESTS_FAILED -eq 0 ]] && return 0 || return 1
    }
}

# Test setup
test_setup() {
    set -x
    # Create test directory
    export SENTINEL_TOOLS_DIR="/tmp/sentinel_tools_test_$$"
    export SENTINEL_TOOLS_REGISTRY="$SENTINEL_TOOLS_DIR/registry.json"
    export SENTINEL_TOOLS_SANDBOX_DIR="$SENTINEL_TOOLS_DIR/sandbox"
    export SENTINEL_TOOLS_PLUGINS_DIR="$SENTINEL_TOOLS_DIR/plugins"
    export SENTINEL_TOOLS_ALLOWED_COMMANDS="git,docker,kubectl,terraform,ansible,mocktool,infotest"
    
    # Disable MCP for testing
    export SENTINEL_MCP_ENABLED=0
    
    # Source the module
    source "$(dirname "$0")/../bash_modules.d/logging.module"
    source "$(dirname "$0")/../bash_modules.d/config_cache.module"
    source "$(dirname "$0")/../bash_modules.d/external_tools.module"
}

# Test cleanup
test_cleanup() {
    rm -rf "$SENTINEL_TOOLS_DIR"
}

# Tests
test_module_loading() {
    test_case "module loading"
    
    if [[ -n "${_SENTINEL_EXTERNAL_TOOLS_LOADED}" ]]; then
        test_pass
    else
        test_fail "Module not loaded"
    fi
}

test_directory_creation() {
    test_case "directory creation"
    
    if [[ -d "$SENTINEL_TOOLS_DIR" && -d "$SENTINEL_TOOLS_SANDBOX_DIR" && -d "$SENTINEL_TOOLS_PLUGINS_DIR" ]]; then
        test_pass
    else
        test_fail "Directories not created"
    fi
}

test_registry_initialization() {
    test_case "registry initialization"
    
    if [[ -f "$SENTINEL_TOOLS_REGISTRY" ]]; then
        local version=$(jq -r '.version' "$SENTINEL_TOOLS_REGISTRY" 2>/dev/null)
        if [[ "$version" == "1.0.0" ]]; then
            test_pass
        else
            test_fail "Invalid registry version"
        fi
    else
        test_fail "Registry not created"
    fi
}

test_tool_registration() {
    test_case "tool registration"
    
    # Create a mock tool
    local mock_tool="/tmp/mocktool"
    echo '#!/bin/bash' > "$mock_tool"
    echo 'echo "mock tool"' >> "$mock_tool"
    chmod +x "$mock_tool"
    
    # Register the tool
    if sentinel_tool_register "mocktool" "$mock_tool" '{"type":"test"}'; then
        # Check if registered
        local registered=$(jq -r '.tools.mocktool.path' "$SENTINEL_TOOLS_REGISTRY" 2>/dev/null)
        if [[ "$registered" == "$mock_tool" ]]; then
            test_pass
        else
            test_fail "Tool not in registry"
        fi
    else
        test_fail "Registration failed"
    fi
    
    rm -f "$mock_tool"
}

test_tool_verification() {
    test_case "tool verification"
    
    # Test allowed command
    if which git >/dev/null 2>&1; then
        if sentinel_tool_verify "$(which git)" >/dev/null 2>&1; then
            test_pass
        else
            test_fail "Failed to verify allowed tool"
        fi
    else
        # Skip test if git not available
        echo "SKIPPED (git not found)"
    fi
}

test_sandbox_wrapper() {
    test_case "sandbox wrapper creation"
    
    # Create mock tool
    local mock_tool="/tmp/mock_sandbox_$$"
    echo '#!/bin/bash' > "$mock_tool"
    echo 'echo "$@"' >> "$mock_tool"
    chmod +x "$mock_tool"
    
    # Create wrapper
    sentinel_tool_create_sandbox_wrapper "mockwrap" "$mock_tool"
    
    if [[ -x "$SENTINEL_TOOLS_SANDBOX_DIR/mockwrap" ]]; then
        test_pass
    else
        test_fail "Wrapper not created"
    fi
    
    rm -f "$mock_tool"
}

test_tool_list() {
    test_case "tool listing"
    
    # List tools and check output
    if sentinel_tool_list json >/dev/null 2>&1; then
        test_pass
    else
        test_fail "List command failed"
    fi
}

test_tool_info() {
    test_case "tool info retrieval"
    
    # Register a test tool first
    local mock_tool="/tmp/infotest"
    touch "$mock_tool"
    chmod +x "$mock_tool"
    
    sentinel_tool_register "infotest" "$mock_tool" '{"type":"test"}' >/dev/null 2>&1
    
    local info=$(sentinel_tool_info "infotest")
    if [[ -n "$info" ]] && [[ "$info" != *"error"* ]]; then
        test_pass
    else
        test_fail "Failed to get tool info"
    fi
    
    rm -f "$mock_tool"
}

test_plugin_structure() {
    test_case "plugin structure"
    
    # Create a test plugin
    cat > "$SENTINEL_TOOLS_PLUGINS_DIR/test.plugin" <<'EOF'
#!/usr/bin/env bash
SENTINEL_PLUGIN_NAME="test"
SENTINEL_PLUGIN_VERSION="1.0.0"
test_plugin_init() { return 0; }
test_plugin_init
EOF
    chmod +x "$SENTINEL_TOOLS_PLUGINS_DIR/test.plugin"
    
    if sentinel_plugin_load "test" >/dev/null 2>&1; then
        test_pass
    else
        test_fail "Plugin load failed"
    fi
}

test_tool_alias() {
    test_case "tool alias creation"
    
    # Create alias
    sentinel_tool_alias "testalias" "echo" "test message"
    
    # Test if function exists
    if type testalias >/dev/null 2>&1; then
        test_pass
    else
        test_fail "Alias not created"
    fi
}

# Main test execution
main() {
    test_start
    test_setup
    
    # Run tests
    test_module_loading
    test_directory_creation
    test_registry_initialization
    test_tool_registration
    test_tool_verification
    test_sandbox_wrapper
    test_tool_list
    test_tool_info
    test_plugin_structure
    test_tool_alias
    
    test_cleanup
    test_summary
}

# Run tests
main "$@"