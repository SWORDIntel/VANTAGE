# SENTINEL Graceful Degradation Strategies

## Overview

Graceful degradation ensures SENTINEL remains functional even when components fail. This document outlines strategies for maintaining core functionality while non-critical features may be unavailable.

## Degradation Hierarchy

### Feature Classification

Features are classified into three criticality levels:

1. **Core Features** (Always Available)
   - Basic command execution
   - File system navigation
   - Essential aliases and functions
   - Basic tab completion
   - Security fundamentals

2. **Important Features** (Available in Graceful/Minimal modes)
   - Enhanced completion
   - Logging system
   - Module loading
   - Basic error handling
   - Performance optimizations

3. **Optional Features** (Available only in Graceful mode)
   - ML/AI suggestions
   - Advanced analytics
   - External integrations
   - Cosmetic enhancements
   - Experimental features

## Module-Specific Fallback Strategies

### 1. Completion System Fallbacks

```bash
# Primary: FZF-based completion
fzf_completion_fallback() {
    # Fallback to bash built-in completion
    echo "FZF unavailable, using standard completion" >&2
    complete -F _minimal_completion -D
}

_minimal_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    COMPREPLY=( $(compgen -f -- "$cur") )
}
```

### 2. ML/AI Feature Fallbacks

```bash
# Primary: ML-based command suggestions
ml_suggestions_fallback() {
    # Fallback to history-based suggestions
    echo "ML unavailable, using history-based suggestions" >&2
    
    suggest_from_history() {
        history | tail -20 | cut -c 8-
    }
    
    alias suggest='suggest_from_history'
}
```

### 3. Logging System Fallbacks

```bash
# Primary: Advanced logging with rotation
logging_fallback() {
    # Fallback to simple echo-based logging
    sentinel_log() {
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >&2
    }
    
    sentinel_log_error() { sentinel_log "ERROR: $*"; }
    sentinel_log_warning() { sentinel_log "WARN: $*"; }
    sentinel_log_info() { sentinel_log "INFO: $*"; }
}
```

### 4. Security Module Fallbacks

```bash
# Primary: HMAC verification
hmac_fallback() {
    # Fallback to basic checksum verification
    echo "HMAC unavailable, using SHA256 checksums" >&2
    
    verify_module() {
        local module="$1"
        sha256sum -c "${module}.sha256" 2>/dev/null
    }
}
```

## Implementation Patterns

### 1. Feature Detection Pattern

```bash
# Check for feature availability before use
load_feature() {
    local feature_name="$1"
    local criticality="$2"
    
    if sentinel_feature_available "$feature_name" "$criticality"; then
        # Load the feature
        source "${SENTINEL_MODULES_PATH}/${feature_name}.module"
    else
        # Use fallback or skip
        local fallback="${feature_name}_fallback"
        if type "$fallback" &>/dev/null; then
            "$fallback"
        fi
    fi
}
```

### 2. Progressive Enhancement Pattern

```bash
# Start with basic functionality, enhance if available
setup_completion() {
    # Basic completion (always available)
    complete -F _basic_completion -D
    
    # Enhanced completion (if available)
    if sentinel_feature_available "advanced_completion" "important"; then
        source ~/.bash_completion
    fi
    
    # FZF completion (if available)
    if sentinel_feature_available "fzf_completion" "optional"; then
        source /usr/share/fzf/completion.bash
    fi
}
```

### 3. Lazy Loading Pattern

```bash
# Defer loading until actually needed
setup_lazy_feature() {
    local feature="$1"
    local command="$2"
    
    # Create wrapper function
    eval "${command}() {
        if sentinel_feature_available '$feature' 'optional'; then
            unset -f $command
            source '${SENTINEL_MODULES_PATH}/${feature}.module'
            $command \"\$@\"
        else
            echo '${command}: Feature unavailable in current mode' >&2
            return 1
        fi
    }"
}

# Example usage
setup_lazy_feature "ml_predict" "predict_next_command"
```

## Mode Transition Strategies

### Automatic Mode Detection

```bash
# Monitor system health and adjust mode
monitor_system_health() {
    local error_count=0
    local threshold=10
    
    # Count recent errors
    error_count=$(sentinel_show_logs all 100 2>/dev/null | grep -c ERROR || echo 0)
    
    # Adjust mode based on health
    if (( error_count > threshold )); then
        sentinel_set_degradation_mode "minimal"
        echo "System experiencing issues, switching to minimal mode" >&2
    elif (( error_count > threshold/2 )); then
        sentinel_set_degradation_mode "graceful"
    fi
}

# Run health check periodically
[[ -z "$SENTINEL_HEALTH_CHECK_PID" ]] && {
    (
        while sleep 300; do
            monitor_system_health
        done
    ) &
    export SENTINEL_HEALTH_CHECK_PID=$!
}
```

### Manual Mode Control

```bash
# User commands for mode control
alias sentinel-mode-safe='sentinel_set_degradation_mode safe'
alias sentinel-mode-minimal='sentinel_set_degradation_mode minimal'
alias sentinel-mode-full='sentinel_set_degradation_mode graceful'
alias sentinel-mode-status='sentinel_error_recovery_status'
```

## Testing Degradation

### Degradation Test Suite

```bash
# Test different degradation scenarios
test_degradation_modes() {
    local original_mode="$SENTINEL_FALLBACK_MODE"
    
    echo "Testing degradation modes..."
    
    for mode in safe minimal graceful; do
        echo "Testing $mode mode..."
        sentinel_set_degradation_mode "$mode"
        
        # Test core features
        echo -n "  Core features: "
        cd /tmp && pwd >/dev/null && echo "OK" || echo "FAIL"
        
        # Test important features
        echo -n "  Important features: "
        if [[ "$mode" != "safe" ]]; then
            type module_enable &>/dev/null && echo "OK" || echo "FAIL"
        else
            echo "SKIPPED"
        fi
        
        # Test optional features
        echo -n "  Optional features: "
        if [[ "$mode" == "graceful" ]]; then
            type predict_next_command &>/dev/null && echo "OK" || echo "FAIL"
        else
            echo "SKIPPED"
        fi
    done
    
    # Restore original mode
    sentinel_set_degradation_mode "$original_mode"
}
```

### Failure Simulation

```bash
# Simulate component failures
simulate_component_failure() {
    local component="$1"
    local failure_count="${2:-10}"
    
    echo "Simulating $failure_count failures for $component..."
    
    for ((i=1; i<=failure_count; i++)); do
        sentinel_circuit_breaker_failure "$component" "Simulated failure $i"
        sleep 0.1
    done
    
    echo "Component $component status:"
    echo "  State: ${SENTINEL_CIRCUIT_BREAKERS[$component]}"
    echo "  Failures: ${SENTINEL_CIRCUIT_BREAKER_FAILURES[$component]}"
}
```

## Best Practices

### 1. Fallback Quality

- Fallbacks should provide essential functionality
- User should be notified when using fallback
- Performance degradation should be graceful
- Data integrity must be maintained

### 2. Mode Selection

- Default to graceful mode for best experience
- Switch to minimal mode during issues
- Use safe mode only when critical
- Allow user override of automatic decisions

### 3. Feature Dependencies

- Map feature dependencies clearly
- Test all fallback paths
- Document degraded functionality
- Provide upgrade paths

### 4. User Communication

```bash
# Notify user of degraded functionality
notify_degradation() {
    local feature="$1"
    local reason="$2"
    
    if [[ -t 2 ]]; then  # Interactive terminal
        echo "⚠️  $feature running in degraded mode: $reason" >&2
    fi
    
    # Log for non-interactive sessions
    sentinel_log_warning "degradation" "$feature degraded: $reason"
}
```

## Recovery Strategies

### Automatic Recovery Attempts

```bash
# Periodic recovery attempts
attempt_recovery() {
    local component="$1"
    
    # Check if circuit breaker can be reset
    if sentinel_circuit_breaker_check "$component"; then
        echo "Attempting recovery for $component..."
        
        # Try to reinitialize
        if type "${component}_init" &>/dev/null; then
            "${component}_init" && {
                echo "$component recovered successfully"
                sentinel_circuit_breaker_success "$component"
            }
        fi
    fi
}
```

### Manual Recovery

```bash
# User-initiated recovery
sentinel_recover() {
    local component="${1:-all}"
    
    if [[ "$component" == "all" ]]; then
        # Recover all components
        for comp in "${!SENTINEL_CIRCUIT_BREAKERS[@]}"; do
            attempt_recovery "$comp"
        done
    else
        attempt_recovery "$component"
    fi
}
```

## Performance Considerations

### Degradation Performance Matrix

| Feature Type | Full Mode | Minimal Mode | Safe Mode |
|-------------|-----------|--------------|-----------|
| Command Execution | 100% | 100% | 100% |
| Tab Completion | Enhanced | Basic | Minimal |
| Command Suggestions | ML-based | History | None |
| Error Handling | Full context | Basic | Minimal |
| Logging | Structured | Simple | Console only |

### Resource Usage

- Safe mode: ~10MB RAM, minimal CPU
- Minimal mode: ~25MB RAM, low CPU
- Graceful mode: ~100MB RAM, normal CPU

## Monitoring and Alerts

### Health Dashboard

```bash
# Display system health dashboard
sentinel_health_dashboard() {
    clear
    echo "=== SENTINEL Health Dashboard ==="
    echo "Mode: $SENTINEL_FALLBACK_MODE"
    echo ""
    echo "Component Status:"
    printf "%-20s %-10s %s\n" "Component" "State" "Failures"
    printf "%-20s %-10s %s\n" "--------" "-----" "--------"
    
    for component in "${!SENTINEL_CIRCUIT_BREAKERS[@]}"; do
        printf "%-20s %-10s %d\n" \
            "$component" \
            "${SENTINEL_CIRCUIT_BREAKERS[$component]}" \
            "${SENTINEL_CIRCUIT_BREAKER_FAILURES[$component]:-0}"
    done
    
    echo ""
    echo "Recent Errors:"
    sentinel_show_logs all 10 2>/dev/null | grep ERROR | tail -5
}
```

## Conclusion

Graceful degradation ensures SENTINEL remains usable under all conditions. By classifying features, implementing fallbacks, and providing clear communication, users maintain productivity even during system issues.