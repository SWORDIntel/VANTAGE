#!/usr/bin/env bash

# Test module loading with dependency resolution

echo "=== SENTINEL Module Loading Test ==="
echo

export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set up environment
export SENTINEL_MODULES_PATH="${SENTINEL_ROOT}/bash_modules.d"
export MODULES_DIR="$SENTINEL_MODULES_PATH"
export SENTINEL_DEBUG_MODULES=0
export SENTINEL_QUIET_MODULES=1
export SENTINEL_SKIP_AUTO_LOAD=1

# Source the bash_modules loader
echo "Loading module system..."
source "${SENTINEL_ROOT}/bash_modules"

echo
echo "=== Testing Module Dependencies ==="
echo

# Test loading modules with dependencies
reset_module_state() {
    local module_name normalized_name
    for module_name in "$@"; do
        normalized_name="${module_name^^}"
        normalized_name="${normalized_name//[^A-Z0-9]/_}"
        unset "_MODULE_${normalized_name}_LOADED"
        unset "_SENTINEL_${normalized_name}_LOADED"
        unset "SENTINEL_LOADED_MODULES[$module_name]"
    done
}

test_module_loading() {
    local module="$1"
    local output=""
    local tmp_output=""
    echo -n "Testing $module... "
    
    # Clear loaded modules
    unset SENTINEL_LOADED_MODULES
    declare -gA SENTINEL_LOADED_MODULES
    reset_module_state logging config_cache module_manager sentinel_markov auto_install command_chains "$module"
    
    # Try to load the module
    tmp_output="$(mktemp)"
    module_enable "$module" 0 "test" >"$tmp_output" 2>&1
    output="$(<"$tmp_output")"
    rm -f "$tmp_output"
    if grep -q "ERROR" <<< "$output"; then
        echo "FAILED"
        return 1
    else
        echo "OK"
        
        # Check if dependencies were loaded
        local module_file="$SENTINEL_MODULES_PATH/$module.module"
        [[ ! -f "$module_file" ]] && module_file="$SENTINEL_MODULES_PATH/$module.sh"
        
        if [[ -f "$module_file" ]] && grep -q "SENTINEL_MODULE_DEPENDENCIES=" "$module_file"; then
            local deps=$(grep "SENTINEL_MODULE_DEPENDENCIES=" "$module_file" | head -n1 | sed 's/.*="\(.*\)".*/\1/')
            
            for dep in $deps; do
                if [[ "${SENTINEL_LOADED_MODULES[$dep]}" == "1" ]]; then
                    echo "  ✓ Dependency $dep loaded successfully"
                else
                    echo "  ✗ Dependency $dep NOT loaded!"
                fi
            done
        fi
        return 0
    fi
}

# Test modules with dependencies
echo "1. Testing config_cache (depends on logging):"
test_module_loading "config_cache"

echo
echo "2. Testing module_manager (depends on config_cache and logging):"
test_module_loading "module_manager"

echo
echo "3. Testing sentinel_markov (depends on logging and config_cache):"
test_module_loading "sentinel_markov"

echo
echo "4. Testing auto_install (depends on logging):"
test_module_loading "auto_install"

echo
echo "5. Testing command_chains (depends on logging):"
test_module_loading "command_chains"

echo
echo "=== Module Loading Order Test ==="
echo

# Test loading all modules in order from .bash_modules
echo "Loading all modules from .bash_modules..."
unset SENTINEL_LOADED_MODULES
declare -gA SENTINEL_LOADED_MODULES
reset_module_state logging config_cache module_manager sentinel_markov auto_install command_chains

# Source the function to load enabled modules
unset SENTINEL_SKIP_AUTO_LOAD
_load_enabled_modules >/dev/null 2>&1

echo
echo "Loaded modules:"
for module in "${!SENTINEL_LOADED_MODULES[@]}"; do
    if [[ "${SENTINEL_LOADED_MODULES[$module]}" == "1" ]]; then
        echo "  ✓ $module"
    fi
done

echo
echo "=== Summary ==="
echo "Module loading test complete. Check for any errors above."
