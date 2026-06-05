#!/usr/bin/env bash
# Test script for SENTINEL module lazy loading

echo "=== SENTINEL Module Lazy Loading Test ==="
echo

# Source the module system
export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${SENTINEL_ROOT}/bash_modules.d/module_manager.module"
source "${SENTINEL_ROOT}/bash_modules"
_load_enabled_modules

echo "1. Checking lazy loading configuration..."
echo "   Lazy loading enabled: ${SENTINEL_LAZY_LOADING_ENABLED:-1}"
echo "   Core modules: ${#SENTINEL_CORE_MODULES[@]}"
echo "   Lazy modules: ${#SENTINEL_LAZY_LOAD_MODULES[@]}"
echo

echo "2. Testing module classification..."
for module in autocomplete sentinel_ml fzf sentinel_osint; do
    if is_lazy_load_module "$module"; then
        echo "   [$module] -> LAZY"
    else
        echo "   [$module] -> EAGER"
    fi
done
echo

echo "3. Checking loaded modules before any action..."
echo "   Total loaded: ${#SENTINEL_LOADED_MODULES[@]}"
for module in "${!SENTINEL_LOADED_MODULES[@]}"; do
    echo "   - $module: ${SENTINEL_LOADED_MODULES[$module]}"
done
echo

echo "4. Checking lazy module registry..."
echo "   Total lazy: ${#SENTINEL_LAZY_MODULES[@]}"
for module in "${!SENTINEL_LAZY_MODULES[@]}"; do
    echo "   - $module: ${SENTINEL_LAZY_MODULES[$module]}"
done
echo

echo "5. Testing proxy functions..."
if type ml_suggest &>/dev/null 2>&1; then
    echo "   ✓ ml_suggest proxy exists"
else
    echo "   ✗ ml_suggest proxy missing"
fi

if type osint &>/dev/null 2>&1; then
    echo "   ✓ osint proxy exists"
else
    echo "   ✗ osint proxy missing"
fi

if type chat &>/dev/null 2>&1; then
    echo "   ✓ chat proxy exists"
else
    echo "   ✗ chat proxy missing"
fi
echo

echo "6. Module status summary:"
module_lazy_status
echo

echo "7. Testing lazy load trigger (dry run)..."
echo "   Would run: ml_suggest test"
echo "   Would run: osint --help"
echo "   Would run: chat --version"
echo

echo "=== Test Complete ==="
echo
echo "To test actual lazy loading:"
echo "  1. Open a new shell"
echo "  2. Run: time bash -c 'source ~/.bashrc; exit'"
echo "  3. Compare with: SENTINEL_LAZY_LOADING_ENABLED=0 time bash -c 'source ~/.bashrc; exit'"