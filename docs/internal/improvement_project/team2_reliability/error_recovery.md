# SENTINEL Error Recovery System

## Overview

The SENTINEL Error Recovery Module provides robust error handling with circuit breaker patterns, graceful degradation, and comprehensive error context preservation. This ensures the system remains usable even during component failures.

## Key Features

### 1. Circuit Breaker Pattern

The circuit breaker pattern prevents cascading failures by:
- Monitoring component failures
- Opening the circuit after threshold failures
- Failing fast to prevent resource exhaustion
- Automatically attempting recovery after timeout

**States:**
- **Closed**: Normal operation, requests pass through
- **Open**: Threshold exceeded, requests fail immediately
- **Half-Open**: Testing if component has recovered

### 2. Error Context Preservation

Captures comprehensive debugging information:
- Call stack at time of error
- Environment variables and state
- Recent command history
- Module loading states
- Circuit breaker states

### 3. Fallback Mechanisms

Allows registration of fallback functions for critical components:
- Automatic fallback triggering on circuit breaker open
- Manual fallback registration
- Graceful feature degradation

### 4. Degradation Modes

Three levels of system degradation:
- **Graceful**: All features available, non-critical may fail
- **Minimal**: Only core and important features enabled
- **Safe**: Maximum stability, only core features

## Usage Examples

### Basic Circuit Breaker Protection

```bash
# Execute command with circuit breaker protection
sentinel_with_circuit_breaker "my_component" my_risky_function arg1 arg2

# The circuit breaker will:
# - Track failures
# - Open after 5 failures (default)
# - Fail fast when open
# - Retry after 5 minutes (default)
```

### Registering Fallbacks

```bash
# Define a fallback function
my_component_fallback() {
    echo "Using simplified fallback functionality"
    # Provide minimal working implementation
    return 0
}

# Register the fallback
sentinel_register_fallback "my_component" "my_component_fallback"

# Now if my_component fails, fallback will be triggered automatically
```

### Safe Module Loading

```bash
# Define module fallback
fzf_fallback() {
    echo "FZF not available, using basic completion"
    # Set up basic completion instead
}

# Load module with fallback
sentinel_safe_module_load "fzf" "fzf_fallback"
```

### Setting Degradation Mode

```bash
# Set to minimal mode during issues
sentinel_set_degradation_mode "minimal"

# Check if feature should be enabled
if sentinel_feature_available "ml_suggestions" "optional"; then
    # Load ML features
    load_ml_features
fi
```

### Error Reporting

```bash
# Generate comprehensive error report
sentinel_generate_error_report

# Check error recovery status
sentinel_error_recovery_status

# View specific error context
cat ~/.sentinel/error_recovery/my_component_*.context
```

## Configuration

Environment variables for customization:

```bash
# Circuit breaker threshold (default: 5)
export SENTINEL_CIRCUIT_BREAKER_THRESHOLD=3

# Circuit breaker timeout in seconds (default: 300)
export SENTINEL_CIRCUIT_BREAKER_TIMEOUT=600

# Error context lines to capture (default: 10)
export SENTINEL_ERROR_CONTEXT_LINES=20

# Enable automatic error reporting (default: 1)
export SENTINEL_ERROR_REPORT_ENABLED=1

# Default fallback mode (default: graceful)
export SENTINEL_FALLBACK_MODE=minimal
```

## Integration with Module System

The error recovery module integrates seamlessly with the module manager:

```bash
# In module_manager.module, wrap loads with circuit breaker
smart_module_load() {
    local module_name="$1"
    
    # Use error recovery if available
    if type sentinel_with_circuit_breaker &>/dev/null; then
        sentinel_with_circuit_breaker "module_$module_name" \
            _original_module_load "$module_name"
    else
        _original_module_load "$module_name"
    fi
}
```

## Best Practices

1. **Register Fallbacks for Critical Components**
   - Always provide fallbacks for essential features
   - Fallbacks should provide minimal but functional alternatives

2. **Use Appropriate Degradation Modes**
   - Start with graceful mode
   - Switch to minimal during issues
   - Use safe mode only for critical stability issues

3. **Monitor Circuit Breaker States**
   - Regularly check `sentinel_error_recovery_status`
   - Investigate components frequently in open state
   - Adjust thresholds based on component reliability

4. **Preserve Error Context**
   - Error contexts are automatically captured
   - Review contexts when debugging issues
   - Clean up old contexts periodically

## Troubleshooting

### Circuit Breaker Stuck Open

```bash
# Manually reset circuit breaker
SENTINEL_CIRCUIT_BREAKERS["component_name"]="closed"
SENTINEL_CIRCUIT_BREAKER_FAILURES["component_name"]=0
```

### Generate Debug Report

```bash
# Full system debug report
sentinel_generate_error_report /tmp/sentinel_debug.txt
cat /tmp/sentinel_debug.txt
```

### Check Feature Availability

```bash
# List all features and their availability
for feature in ml ai completion security; do
    for level in core important optional; do
        if sentinel_feature_available "$feature" "$level"; then
            echo "$feature ($level): available"
        else
            echo "$feature ($level): disabled"
        fi
    done
done
```

## Architecture

The error recovery system uses several design patterns:

1. **Circuit Breaker**: Prevents cascading failures
2. **Fallback**: Provides alternative implementations
3. **Graceful Degradation**: Reduces functionality to maintain stability
4. **Error Context**: Preserves debugging information
5. **State Management**: Tracks component health

## Performance Impact

The error recovery system has minimal performance impact:
- Circuit breaker checks: < 1ms
- Error context capture: < 10ms (done only on failure)
- Fallback execution: Depends on fallback implementation
- No impact on successful operations

## Future Enhancements

Planned improvements:
- Automatic fallback discovery
- Machine learning for failure prediction
- Distributed circuit breaker state
- Enhanced error analytics
- Self-healing capabilities