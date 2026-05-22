# SENTINEL Error Recovery Integration Example

This document demonstrates how to integrate the error recovery and graceful degradation system into existing SENTINEL modules.

## Integration Steps

### 1. Update Module Dependencies

Add `error_recovery` to your module's dependencies:

```bash
SENTINEL_MODULE_DEPENDENCIES="error_recovery logging"
```

### 2. Implement Fallback Functions

Create fallback implementations for critical functionality:

```bash
# In your module file:

# Primary function
advanced_feature() {
    # Complex implementation requiring external tools
    fzf --preview 'cat {}' --height 40%
}

# Fallback function
advanced_feature_fallback() {
    sentinel_log_warning "my_module" "Using fallback for advanced_feature"
    # Simple alternative implementation
    select file in $(ls); do
        [[ -n "$file" ]] && echo "$file" && break
    done
}

# Register the fallback
if type sentinel_register_fallback &>/dev/null; then
    sentinel_register_fallback "advanced_feature" "advanced_feature_fallback"
fi
```

### 3. Wrap Critical Operations

Use circuit breaker protection for operations that might fail:

```bash
# Protect external command execution
process_file() {
    local file="$1"
    
    if type sentinel_with_circuit_breaker &>/dev/null; then
        sentinel_with_circuit_breaker "file_processor" \
            _do_process_file "$file"
    else
        _do_process_file "$file"
    fi
}

_do_process_file() {
    local file="$1"
    # Actual processing logic
    external_tool --process "$file"
}
```

### 4. Check Feature Availability

Respect degradation modes when loading features:

```bash
# In module initialization
init_module() {
    # Always load core features
    load_core_features
    
    # Load optional features based on mode
    if sentinel_feature_available "ml_integration" "optional"; then
        load_ml_features
    fi
    
    if sentinel_feature_available "advanced_ui" "important"; then
        load_advanced_ui
    fi
}
```

## Complete Module Example

Here's a complete example of a module with error recovery integration:

```bash
#!/usr/bin/env bash
# SENTINEL Example Module with Error Recovery
# Version: 1.0.0
# Description: Demonstrates error recovery integration
# Dependencies: error_recovery logging

SENTINEL_MODULE_DESCRIPTION="Example module with error recovery"
SENTINEL_MODULE_VERSION="1.0.0"
SENTINEL_MODULE_DEPENDENCIES="error_recovery logging"

# Prevent double loading
[[ -n "${_SENTINEL_EXAMPLE_LOADED}" ]] && return 0
export _SENTINEL_EXAMPLE_LOADED=1

# Configuration
: "${EXAMPLE_CACHE_DIR:=$HOME/.sentinel/example}"
: "${EXAMPLE_TIMEOUT:=30}"

# Initialize module with error handling
init_example_module() {
    # Create cache directory with fallback
    if ! mkdir -p "$EXAMPLE_CACHE_DIR" 2>/dev/null; then
        EXAMPLE_CACHE_DIR="/tmp/sentinel_example_$$"
        mkdir -p "$EXAMPLE_CACHE_DIR"
        sentinel_log_warning "example" "Using temporary cache directory"
    fi
    
    # Initialize circuit breaker for external API
    sentinel_circuit_breaker_init "example_api" 3 300
    
    # Load features based on degradation mode
    if sentinel_feature_available "example_advanced" "optional"; then
        load_advanced_features
    else
        sentinel_log_info "example" "Advanced features disabled in current mode"
    fi
    
    sentinel_log_info "example" "Example module initialized"
}

# Advanced feature with fallback
fetch_data() {
    local query="$1"
    
    # Try with circuit breaker protection
    if sentinel_with_circuit_breaker "example_api" \
        _fetch_data_api "$query"; then
        return 0
    else
        # API failed, use fallback
        fetch_data_fallback "$query"
    fi
}

_fetch_data_api() {
    local query="$1"
    
    # Simulate API call
    curl -s --max-time "$EXAMPLE_TIMEOUT" \
        "https://api.example.com/data?q=$query" || return 1
}

fetch_data_fallback() {
    local query="$1"
    
    sentinel_log_warning "example" "Using cached data for query: $query"
    
    # Try to use cached data
    local cache_file="$EXAMPLE_CACHE_DIR/$(echo "$query" | md5sum | cut -d' ' -f1)"
    if [[ -f "$cache_file" ]]; then
        cat "$cache_file"
    else
        echo "No cached data available for: $query" >&2
        return 1
    fi
}

# Process with error context
process_data() {
    local input="$1"
    
    # Capture pre-execution context
    sentinel_capture_error_context "example_process" "pre-execution"
    
    # Process with error handling
    if ! _do_process "$input"; then
        # Capture failure context
        sentinel_capture_error_context "example_process" "post-failure"
        
        # Try recovery
        if type process_data_recovery &>/dev/null; then
            process_data_recovery "$input"
        else
            return 1
        fi
    fi
}

_do_process() {
    local input="$1"
    # Complex processing that might fail
    [[ -n "$input" ]] || return 1
    echo "Processing: $input"
}

process_data_recovery() {
    local input="$1"
    
    sentinel_log_info "example" "Attempting data processing recovery"
    
    # Simple recovery logic
    echo "Recovered: $input (simplified)"
}

# Advanced UI feature (optional)
load_advanced_features() {
    if command -v fzf &>/dev/null; then
        example_interactive_select() {
            fzf --height 40% --reverse
        }
    else
        # Register fallback
        sentinel_register_fallback "example_interactive" \
            "example_interactive_fallback"
        
        example_interactive_select() {
            example_interactive_fallback
        }
    fi
}

example_interactive_fallback() {
    sentinel_log_warning "example" "Using basic selection (FZF unavailable)"
    
    select item in $(cat); do
        [[ -n "$item" ]] && echo "$item" && break
    done
}

# Health check function
example_health_check() {
    local status=0
    
    echo "Example Module Health Check:"
    
    # Check cache directory
    if [[ -d "$EXAMPLE_CACHE_DIR" ]] && [[ -w "$EXAMPLE_CACHE_DIR" ]]; then
        echo "  ✓ Cache directory: OK"
    else
        echo "  ✗ Cache directory: FAIL"
        ((status++))
    fi
    
    # Check circuit breaker state
    local cb_state="${SENTINEL_CIRCUIT_BREAKERS[example_api]:-unknown}"
    echo "  - API circuit breaker: $cb_state"
    
    # Check feature availability
    if sentinel_feature_available "example_advanced" "optional"; then
        echo "  ✓ Advanced features: Available"
    else
        echo "  - Advanced features: Disabled"
    fi
    
    return $status
}

# Register all fallbacks
register_example_fallbacks() {
    sentinel_register_fallback "fetch_data" "fetch_data_fallback"
    sentinel_register_fallback "process_data" "process_data_recovery"
    sentinel_register_fallback "example_interactive" "example_interactive_fallback"
}

# Export functions
export -f fetch_data
export -f process_data
export -f example_interactive_select
export -f example_health_check

# Initialize module
init_example_module
register_example_fallbacks
```

## Testing Error Recovery

### Manual Testing

```bash
# Test circuit breaker
for i in {1..5}; do
    fetch_data "test query $i"
done

# Check circuit breaker state
sentinel_error_recovery_status

# Test degradation modes
sentinel_set_degradation_mode "minimal"
example_health_check

sentinel_set_degradation_mode "safe"
example_health_check
```

### Automated Testing

Create a test file for your module:

```bash
#!/usr/bin/env bash
# Test error recovery in example module

# Source modules
source ~/.bash_modules.d/error_recovery.module
source ~/.bash_modules.d/example.module

# Test 1: Circuit breaker functionality
echo "Testing circuit breaker..."
# Force failures
for i in {1..5}; do
    _fetch_data_api "fail" 2>/dev/null
done

# Check if circuit is open
if [[ "${SENTINEL_CIRCUIT_BREAKERS[example_api]}" == "open" ]]; then
    echo "✓ Circuit breaker opened correctly"
else
    echo "✗ Circuit breaker failed to open"
fi

# Test 2: Fallback execution
echo "Testing fallbacks..."
# Create cache file for fallback
echo "cached data" > "$EXAMPLE_CACHE_DIR/$(echo 'test' | md5sum | cut -d' ' -f1)"

# Should use fallback since circuit is open
result=$(fetch_data "test" 2>&1)
if [[ "$result" == *"cached data"* ]]; then
    echo "✓ Fallback executed successfully"
else
    echo "✗ Fallback failed"
fi

# Test 3: Feature availability
echo "Testing feature availability..."
sentinel_set_degradation_mode "safe"
if ! sentinel_feature_available "example_advanced" "optional"; then
    echo "✓ Optional features correctly disabled in safe mode"
else
    echo "✗ Feature availability check failed"
fi
```

## Monitoring and Maintenance

### Dashboard Integration

Add your module to the health dashboard:

```bash
# In ~/.bashrc or monitoring script
sentinel_module_health_check() {
    echo "=== Module Health Status ==="
    
    # Check each module
    for module in example fzf autocomplete ml; do
        if type "${module}_health_check" &>/dev/null 2>&1; then
            "${module}_health_check"
        fi
    done
    
    echo ""
    echo "=== Circuit Breaker Summary ==="
    for component in "${!SENTINEL_CIRCUIT_BREAKERS[@]}"; do
        printf "%-20s: %s\n" "$component" "${SENTINEL_CIRCUIT_BREAKERS[$component]}"
    done
}
```

### Logging Integration

Ensure proper logging for debugging:

```bash
# Use structured logging
sentinel_log_info "example" "Operation started" 
sentinel_log_error "example" "Operation failed: $error_message"
sentinel_log_debug "example" "Debug info: $debug_data"
```

## Best Practices Summary

1. **Always provide fallbacks** for external dependencies
2. **Use circuit breakers** for unreliable operations
3. **Respect degradation modes** when loading features
4. **Capture error context** for debugging
5. **Test all failure paths** including fallbacks
6. **Log appropriately** at each level
7. **Document degraded functionality** for users
8. **Monitor circuit breaker states** regularly

## Troubleshooting

### Common Issues

1. **Circuit breaker stuck open**
   ```bash
   # Manually reset if needed
   SENTINEL_CIRCUIT_BREAKERS["component_name"]="closed"
   SENTINEL_CIRCUIT_BREAKER_FAILURES["component_name"]=0
   ```

2. **Fallback not triggering**
   - Check fallback registration
   - Verify error_recovery module is loaded
   - Check degradation mode settings

3. **Features unavailable**
   - Check current degradation mode
   - Verify feature criticality settings
   - Review circuit breaker states

### Debug Commands

```bash
# Full system status
sentinel_error_recovery_status

# Generate error report
sentinel_generate_error_report

# Check specific component
echo "State: ${SENTINEL_CIRCUIT_BREAKERS[component_name]}"
echo "Failures: ${SENTINEL_CIRCUIT_BREAKER_FAILURES[component_name]}"

# View error contexts
ls -la ~/.sentinel/error_recovery/*.context
```