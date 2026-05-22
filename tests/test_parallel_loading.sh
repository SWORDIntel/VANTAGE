#!/usr/bin/env bash
# Test script for SENTINEL parallel module loading system

set -o pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Test base directory
TEST_DIR="/tmp/sentinel_parallel_test_$$"
mkdir -p "$TEST_DIR/modules"

echo "=== SENTINEL Parallel Loading Test Suite ==="
echo "Test directory: $TEST_DIR"
echo

# Helper functions
pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((++TESTS_PASSED))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((++TESTS_FAILED))
}

info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Create test modules with dependencies
create_test_modules() {
    info "Creating test modules..."
    
    # Module A - no dependencies
    cat > "$TEST_DIR/modules/module_a.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Test module A"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES=""

module_a_loaded=1
sleep 0.1  # Simulate some work
EOF

    # Module B - depends on A
    cat > "$TEST_DIR/modules/module_b.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Test module B"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="module_a"

if [[ "${module_a_loaded:-0}" != "1" ]]; then
    echo "ERROR: module_a not loaded before module_b" >&2
    exit 1
fi
module_b_loaded=1
sleep 0.1
EOF

    # Module C - depends on A
    cat > "$TEST_DIR/modules/module_c.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Test module C"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="module_a"

if [[ "${module_a_loaded:-0}" != "1" ]]; then
    echo "ERROR: module_a not loaded before module_c" >&2
    exit 1
fi
module_c_loaded=1
sleep 0.1
EOF

    # Module D - depends on B and C
    cat > "$TEST_DIR/modules/module_d.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Test module D"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="module_b module_c"

if [[ "${module_b_loaded:-0}" != "1" || "${module_c_loaded:-0}" != "1" ]]; then
    echo "ERROR: Dependencies not loaded before module_d" >&2
    exit 1
fi
module_d_loaded=1
sleep 0.1
EOF

    # Module E - no dependencies (can load in parallel with A)
    cat > "$TEST_DIR/modules/module_e.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Test module E"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES=""

module_e_loaded=1
sleep 0.1
EOF

    chmod +x "$TEST_DIR/modules/"*.module
}

export SENTINEL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

# Test 1: Module metadata extraction
test_metadata_extraction() {
    info "Test 1: Module metadata extraction"
    
    # Source the parallel loader
    export SENTINEL_MODULES_PATH="$TEST_DIR/modules"
    source "${SENTINEL_ROOT}/bash_modules.d/parallel_loader.module"
    clear_module_cache
    
    # Test metadata loading
    _load_module_metadata "module_a" "$TEST_DIR/modules/module_a.module"
    
    if [[ -n "${SENTINEL_MODULE_METADATA_CACHE[module_a]}" ]]; then
        pass "Metadata extracted successfully"
    else
        fail "Metadata extraction failed"
    fi
    
    # Check cache file creation
    if [[ -f "$SENTINEL_MODULE_CACHE_DIR/module_a.meta" ]]; then
        pass "Cache file created"
    else
        fail "Cache file not created"
    fi
}

# Test 2: Dependency graph building
test_dependency_graph() {
    info "Test 2: Dependency graph building"
    
    # Build dependency graph
    _build_dependency_graph "$TEST_DIR/modules"
    
    # Check dependencies
    if [[ "${SENTINEL_MODULE_DEPS_GRAPH[module_d]}" == "module_b module_c" ]]; then
        pass "Dependencies correctly identified"
    else
        fail "Dependencies not correctly identified"
    fi
    
    # Check reverse dependencies
    if [[ "${SENTINEL_MODULE_REVERSE_DEPS[module_a]}" =~ "module_b" ]]; then
        pass "Reverse dependencies correctly built"
    else
        fail "Reverse dependencies not correctly built"
    fi
}

# Test 3: Topological sort
test_topological_sort() {
    info "Test 3: Topological sort"
    
    # Get sorted order
    local -a sorted_order
    mapfile -t sorted_order < <(_topological_sort)
    
    # Check that A comes before B and C
    local a_index=-1
    local b_index=-1
    local d_index=-1
    
    for i in "${!sorted_order[@]}"; do
        case "${sorted_order[$i]}" in
            module_a) a_index=$i ;;
            module_b) b_index=$i ;;
            module_d) d_index=$i ;;
        esac
    done
    
    if [[ $a_index -lt $b_index && $b_index -lt $d_index ]]; then
        pass "Topological sort correct"
    else
        fail "Topological sort incorrect"
    fi
}

# Test 4: Parallel group identification
test_parallel_groups() {
    info "Test 4: Parallel group identification"
    
    # Get parallel groups
    local -a groups
    mapfile -t groups < <(_identify_parallel_groups)
    
    # First group should contain A and E (no dependencies)
    if [[ "${groups[0]}" =~ "module_a" && "${groups[0]}" =~ "module_e" ]]; then
        pass "Independent modules grouped correctly"
    else
        fail "Independent modules not grouped correctly"
    fi
    
    # B and C should be in the same group (both depend only on A)
    local found_bc_group=0
    for group in "${groups[@]}"; do
        if [[ "$group" =~ "module_b" && "$group" =~ "module_c" ]]; then
            found_bc_group=1
            break
        fi
    done
    
    if [[ $found_bc_group -eq 1 ]]; then
        pass "Dependent modules grouped correctly"
    else
        fail "Dependent modules not grouped correctly"
    fi
}

# Test 5: Cache hit performance
test_cache_performance() {
    info "Test 5: Cache hit performance"
    
    # Clear cache
    clear_module_cache
    
    # First load (cache miss)
    local start_time=$(date +%s%N)
    _load_module_metadata "module_a" "$TEST_DIR/modules/module_a.module"
    local end_time=$(date +%s%N)
    local cache_miss_time=$((($end_time - $start_time) / 1000000))
    
    # Second load (cache hit)
    start_time=$(date +%s%N)
    _load_module_metadata "module_a" "$TEST_DIR/modules/module_a.module"
    end_time=$(date +%s%N)
    local cache_hit_time=$((($end_time - $start_time) / 1000000))
    
    info "Cache miss time: ${cache_miss_time}ms"
    info "Cache hit time: ${cache_hit_time}ms"
    
    if [[ "${SENTINEL_MODULE_CACHE_HITS[module_a]:-0}" == "1" ]]; then
        pass "Cache hit recorded successfully"
    else
        fail "Cache hit was not recorded"
    fi
}

# Test 6: Parallel loading execution
test_parallel_loading() {
    info "Test 6: Parallel loading execution"
    
    # Create enabled modules list
    cat > "$TEST_DIR/enabled_modules" << EOF
module_a
module_b
module_c
module_d
module_e
EOF

    # Clear loaded modules
    unset SENTINEL_LOADED_MODULES
    declare -A SENTINEL_LOADED_MODULES
    
    # Measure sequential loading time
    local start_time=$(date +%s%N)
    for module in module_a module_e module_b module_c module_d; do
        source "$TEST_DIR/modules/${module}.module"
    done
    local end_time=$(date +%s%N)
    local sequential_time=$((($end_time - $start_time) / 1000000))
    
    # Clear and test parallel loading
    unset module_a_loaded module_b_loaded module_c_loaded module_d_loaded module_e_loaded
    unset SENTINEL_LOADED_MODULES
    declare -A SENTINEL_LOADED_MODULES
    
    start_time=$(date +%s%N)
    parallel_load_modules "$TEST_DIR/enabled_modules"
    end_time=$(date +%s%N)
    local parallel_time=$((($end_time - $start_time) / 1000000))
    
    info "Sequential loading time: ${sequential_time}ms"
    info "Parallel loading time: ${parallel_time}ms"
    
    # Check all modules loaded
    if [[ "${module_a_loaded:-0}" == "1" && "${module_d_loaded:-0}" == "1" ]]; then
        pass "All modules loaded successfully"
    else
        fail "Not all modules loaded"
    fi
    
    # Parallel should be faster (accounting for overhead)
    if [[ $parallel_time -lt $sequential_time ]]; then
        pass "Parallel loading faster than sequential"
    else
        info "Parallel loading not faster (may be due to overhead on small modules)"
    fi
}

# Test 7: Error handling
test_error_handling() {
    info "Test 7: Error handling"
    
    # Create a module with circular dependency
    cat > "$TEST_DIR/modules/module_circular1.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Circular dependency test 1"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="module_circular2"
EOF

    cat > "$TEST_DIR/modules/module_circular2.module" << 'EOF'
#!/usr/bin/env bash
SENTINEL_MODULE_DESCRIPTION="Circular dependency test 2"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="module_circular1"
EOF

    chmod +x "$TEST_DIR/modules/module_circular"*.module
    
    # Try to build dependency graph (should handle circular deps)
    if _build_dependency_graph "$TEST_DIR/modules" 2>/dev/null; then
        pass "Circular dependency handled gracefully"
    else
        fail "Circular dependency caused failure"
    fi
}

# Run all tests
main() {
    # Create test modules
    create_test_modules
    
    # Run tests
    test_metadata_extraction
    echo
    test_dependency_graph
    echo
    test_topological_sort
    echo
    test_parallel_groups
    echo
    test_cache_performance
    echo
    test_parallel_loading
    echo
    test_error_handling
    echo
    
    # Summary
    echo "=== Test Summary ==="
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    
    # Cleanup
    rm -rf "$TEST_DIR"
    
    # Exit with appropriate code
    if [[ $TESTS_FAILED -eq 0 ]]; then
        echo -e "\n${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "\n${RED}Some tests failed!${NC}"
        exit 1
    fi
}

# Run main
main "$@"
