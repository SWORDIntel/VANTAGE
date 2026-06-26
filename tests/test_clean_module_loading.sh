#!/usr/bin/env bash

# Clean test of module loading with dependency resolution

echo "=== VANTAGE Clean Module Loading Test ==="
echo

export VANTAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set up environment
export VANTAGE_MODULES_PATH="${VANTAGE_ROOT}/bash_modules.d"
export MODULES_DIR="$VANTAGE_MODULES_PATH"
export VANTAGE_DEBUG_MODULES=0
export VANTAGE_QUIET_MODULES=1
export VANTAGE_SKIP_AUTO_LOAD=1  # Prevent automatic loading

# Source only the module functions without loading any modules
echo "Loading module system functions only..."
source "${VANTAGE_ROOT}/bash_modules"

echo
echo "=== Testing Individual Module Loading with Dependencies ==="
echo

# Test loading a module that has dependencies
reset_module_state() {
    local module_name normalized_name
    for module_name in "$@"; do
        normalized_name="${module_name^^}"
        normalized_name="${normalized_name//[^A-Z0-9]/_}"
        unset "_MODULE_${normalized_name}_LOADED"
        unset "_VANTAGE_${normalized_name}_LOADED"
        unset "VANTAGE_LOADED_MODULES[$module_name]"
    done
}

echo "1. Testing config_cache (depends on: logging)"
echo "   Initial state: No modules loaded"
echo

# Clear any loaded modules
unset VANTAGE_LOADED_MODULES
declare -gA VANTAGE_LOADED_MODULES
reset_module_state logging config_cache module_manager vantage_markov

echo "   Loading config_cache..."
module_enable "config_cache" 0 "test" >/dev/null 2>&1

echo
echo "   Loaded modules after loading config_cache:"
for module in "${!VANTAGE_LOADED_MODULES[@]}"; do
    if [[ "${VANTAGE_LOADED_MODULES[$module]}" == "1" ]]; then
        echo "     ✓ $module"
    fi
done

echo
echo "2. Testing module_manager (depends on: config_cache logging)"
echo "   Clearing all loaded modules..."

# Clear loaded modules again
unset VANTAGE_LOADED_MODULES
declare -gA VANTAGE_LOADED_MODULES
reset_module_state logging config_cache module_manager vantage_markov

echo "   Loading module_manager..."
module_enable "module_manager" 0 "test" >/dev/null 2>&1

echo
echo "   Loaded modules after loading module_manager:"
for module in "${!VANTAGE_LOADED_MODULES[@]}"; do
    if [[ "${VANTAGE_LOADED_MODULES[$module]}" == "1" ]]; then
        echo "     ✓ $module"
    fi
done

echo
echo "3. Testing vantage_markov (depends on: logging config_cache)"
echo "   Clearing all loaded modules..."

# Clear loaded modules again
unset VANTAGE_LOADED_MODULES
declare -gA VANTAGE_LOADED_MODULES
reset_module_state logging config_cache module_manager vantage_markov

echo "   Loading vantage_markov..."
module_enable "vantage_markov" 0 "test" >/dev/null 2>&1

echo
echo "   Loaded modules after loading vantage_markov:"
for module in "${!VANTAGE_LOADED_MODULES[@]}"; do
    if [[ "${VANTAGE_LOADED_MODULES[$module]}" == "1" ]]; then
        echo "     ✓ $module"
    fi
done

echo
echo "=== Summary ==="
echo "Dependency resolution test complete."
echo "If dependencies are properly configured, you should see:"
echo "  - config_cache loading should also load: logging"
echo "  - module_manager loading should also load: config_cache and logging"
echo "  - vantage_markov loading should also load: logging and config_cache"
