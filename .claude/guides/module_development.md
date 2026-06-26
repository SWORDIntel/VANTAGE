# VANTAGE Module Development Guide

## Quick Start

Create a new module in 5 minutes:

```bash
# 1. Copy template
cp .claude/templates/module_template.module bash_modules.d/myfeature.module

# 2. Edit metadata
vim bash_modules.d/myfeature.module

# 3. Test loading
bash_modules load myfeature

# 4. Verify functionality
myfeature_test_function
```

## Module Development Workflow

### 1. Planning Phase

Before writing code, consider:
- What problem does this module solve?
- What dependencies does it need?
- How will it impact shell startup time?
- What security implications exist?

### 2. Create Module File

```bash
# Module naming convention:
# - Use lowercase with underscores
# - Be descriptive but concise
# - Examples: git_enhanced, security_audit, ml_predictions

touch bash_modules.d/awesome_feature.module
chmod 644 bash_modules.d/awesome_feature.module
```

### 3. Write Module Code

#### Basic Structure
```bash
#!/bin/bash
# Module: awesome_feature
# Version: 1.0.0
# Description: Adds awesome functionality to VANTAGE
# Dependencies: logging config_cache
# Author: Your Name
# License: Same as VANTAGE

# Prevent direct execution
[[ "${BASH_SOURCE[0]}" == "${0}" ]] && {
    echo "This module should be sourced, not executed directly."
    exit 1
}

# Module metadata (REQUIRED)
MODULE_NAME="awesome_feature"
MODULE_VERSION="1.0.0"
MODULE_DEPS=("logging" "config_cache")
MODULE_DESCRIPTION="Adds awesome functionality"

# Module configuration
MODULE_CONFIG_VARS=(
    "AWESOME_FEATURE_ENABLED:true"
    "AWESOME_FEATURE_LEVEL:medium"
    "AWESOME_FEATURE_TIMEOUT:30"
)

# Module initialization (REQUIRED)
_init_awesome_feature() {
    # Check dependencies
    for dep in "${MODULE_DEPS[@]}"; do
        if ! is_module_loaded "$dep"; then
            log_error "Dependency $dep not loaded"
            return 1
        fi
    done
    
    # Load configuration
    _load_awesome_feature_config
    
    # Initialize module state
    export AWESOME_FEATURE_LOADED=1
    export AWESOME_FEATURE_STATE="initialized"
    
    # Set up aliases/functions
    alias awesome='awesome_feature_main'
    
    log_info "Awesome feature module loaded successfully"
    return 0
}

# Configuration loader
_load_awesome_feature_config() {
    for var_def in "${MODULE_CONFIG_VARS[@]}"; do
        local var_name="${var_def%%:*}"
        local default_val="${var_def#*:}"
        
        # Check environment first, then use default
        local value="${!var_name:-$default_val}"
        export "$var_name=$value"
    done
}

# Main functionality
awesome_feature_main() {
    local action="${1:-help}"
    shift
    
    case "$action" in
        start)
            awesome_feature_start "$@"
            ;;
        stop)
            awesome_feature_stop "$@"
            ;;
        status)
            awesome_feature_status "$@"
            ;;
        help|*)
            awesome_feature_help
            ;;
    esac
}

# Feature implementation
awesome_feature_start() {
    [[ "$AWESOME_FEATURE_ENABLED" != "true" ]] && {
        echo "Awesome feature is disabled"
        return 1
    }
    
    echo "Starting awesome feature..."
    # Implementation here
}

awesome_feature_stop() {
    echo "Stopping awesome feature..."
    # Implementation here
}

awesome_feature_status() {
    echo "Awesome Feature Status:"
    echo "  Enabled: $AWESOME_FEATURE_ENABLED"
    echo "  Level: $AWESOME_FEATURE_LEVEL"
    echo "  State: $AWESOME_FEATURE_STATE"
}

awesome_feature_help() {
    cat << EOF
Awesome Feature Module

Usage: awesome [command] [options]

Commands:
  start   - Start the awesome feature
  stop    - Stop the awesome feature  
  status  - Show current status
  help    - Show this help message

Configuration:
  AWESOME_FEATURE_ENABLED - Enable/disable feature (true/false)
  AWESOME_FEATURE_LEVEL   - Feature level (low/medium/high)
  AWESOME_FEATURE_TIMEOUT - Timeout in seconds

Examples:
  awesome start
  awesome status
  AWESOME_FEATURE_LEVEL=high awesome start
EOF
}

# Cleanup function (OPTIONAL)
_cleanup_awesome_feature() {
    # Clean up resources
    unset AWESOME_FEATURE_LOADED
    unset AWESOME_FEATURE_STATE
    unalias awesome 2>/dev/null
    
    log_info "Awesome feature module cleaned up"
}

# Module self-test (OPTIONAL but recommended)
_test_awesome_feature() {
    local errors=0
    
    # Test 1: Check initialization
    if [[ "$AWESOME_FEATURE_LOADED" != "1" ]]; then
        echo "FAIL: Module not properly initialized"
        ((errors++))
    fi
    
    # Test 2: Check functions exist
    if ! type awesome_feature_main &>/dev/null; then
        echo "FAIL: Main function not defined"
        ((errors++))
    fi
    
    # Test 3: Check configuration
    if [[ -z "$AWESOME_FEATURE_ENABLED" ]]; then
        echo "FAIL: Configuration not loaded"
        ((errors++))
    fi
    
    if [[ $errors -eq 0 ]]; then
        echo "PASS: All tests passed"
        return 0
    else
        echo "FAIL: $errors tests failed"
        return 1
    fi
}

# Auto-init hook
if [[ "${MODULE_AUTO_INIT:-true}" == "true" ]]; then
    _init_awesome_feature
fi
```

### 4. Testing Your Module

#### Unit Testing
```bash
# Create test file
cat > tests/test_awesome_feature.sh << 'EOF'
#!/bin/bash
source bash_modules.d/awesome_feature.module

# Test initialization
_init_awesome_feature || exit 1

# Test functionality
awesome_feature_main status || exit 1

# Run module self-test
_test_awesome_feature || exit 1

echo "All tests passed!"
EOF

chmod +x tests/test_awesome_feature.sh
./tests/test_awesome_feature.sh
```

#### Integration Testing
```bash
# Test in actual shell environment
bash --rcfile <(echo '
source ~/.bashrc
source bash_modules.d/awesome_feature.module
') -i

# In the new shell:
awesome status
```

### 5. Performance Optimization

#### Measure Load Time
```bash
# Add timing to module
_init_awesome_feature() {
    local start_time=$(date +%s%N)
    
    # ... initialization code ...
    
    local end_time=$(date +%s%N)
    local load_time=$(( (end_time - start_time) / 1000000 ))
    log_debug "Module loaded in ${load_time}ms"
}
```

#### Lazy Loading Example
```bash
# Defer expensive operations
_init_awesome_feature() {
    # Just set up the command
    alias awesome='_awesome_lazy_init && awesome'
    return 0
}

_awesome_lazy_init() {
    # Remove the lazy alias
    unalias awesome
    
    # Do the real initialization
    _awesome_real_init
    
    # Create the real alias
    alias awesome='awesome_feature_main'
}
```

### 6. Security Hardening

#### Input Validation
```bash
awesome_feature_process() {
    local input="$1"
    
    # Validate input
    if [[ ! "$input" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        log_error "Invalid input: $input"
        return 1
    fi
    
    # Safe to use input now
    process_data "$input"
}
```

#### Secure Defaults
```bash
# Use secure defaults in configuration
MODULE_CONFIG_VARS=(
    "AWESOME_FEATURE_SECURE_MODE:true"
    "AWESOME_FEATURE_VALIDATE_INPUT:true"
    "AWESOME_FEATURE_LOG_COMMANDS:true"
)
```

### 7. Documentation

#### Inline Documentation
```bash
# Document complex functions
# Function: awesome_feature_complex_operation
# Purpose: Performs complex data transformation
# Arguments:
#   $1 - Input file path (required)
#   $2 - Output format (optional, default: json)
# Returns:
#   0 - Success
#   1 - Invalid input
#   2 - Processing error
# Example:
#   awesome_feature_complex_operation /tmp/data.txt xml
awesome_feature_complex_operation() {
    local input_file="$1"
    local output_format="${2:-json}"
    # ... implementation ...
}
```

#### Module Documentation
Create `docs/modules/awesome_feature.md`:
```markdown
# Awesome Feature Module

## Overview
Brief description of what the module does.

## Features
- Feature 1
- Feature 2
- Feature 3

## Configuration
| Variable | Default | Description |
|----------|---------|-------------|
| AWESOME_FEATURE_ENABLED | true | Enable/disable the feature |
| AWESOME_FEATURE_LEVEL | medium | Feature intensity level |

## Usage Examples
...
```

### 8. Best Practices Checklist

- [ ] Module has proper metadata (name, version, description)
- [ ] Dependencies are declared and checked
- [ ] Functions are namespaced (prefixed with module name)
- [ ] Error handling is comprehensive
- [ ] Logging is used appropriately
- [ ] Module can be loaded/unloaded cleanly
- [ ] Performance impact is minimal (<20ms load time)
- [ ] Security considerations are addressed
- [ ] Documentation is complete
- [ ] Tests are included

### 9. Common Patterns

#### Configuration UI
```bash
_config_awesome_feature() {
    local current_enabled="$AWESOME_FEATURE_ENABLED"
    local current_level="$AWESOME_FEATURE_LEVEL"
    
    echo "Awesome Feature Configuration"
    echo "============================"
    echo "1. Enabled: $current_enabled"
    echo "2. Level: $current_level"
    echo "3. Save and exit"
    echo "4. Cancel"
    
    read -p "Choice: " choice
    # ... handle configuration ...
}
```

#### Event Handling
```bash
# Register for events
_init_awesome_feature() {
    register_event_handler "command_executed" "_awesome_on_command"
    register_event_handler "shell_exit" "_awesome_on_exit"
}

_awesome_on_command() {
    local command="$1"
    # React to command execution
}
```

#### State Persistence
```bash
# Save state between sessions
_awesome_save_state() {
    local state_file="$HOME/.cache/vantage/awesome_feature.state"
    mkdir -p "$(dirname "$state_file")"
    
    cat > "$state_file" << EOF
AWESOME_FEATURE_LAST_RUN=$(date +%s)
AWESOME_FEATURE_COUNTER=$((AWESOME_FEATURE_COUNTER + 1))
EOF
}

# Load state on init
_awesome_load_state() {
    local state_file="$HOME/.cache/vantage/awesome_feature.state"
    [[ -f "$state_file" ]] && source "$state_file"
}
```

### 10. Publishing Your Module

1. **Test Thoroughly**
   - Test on different systems
   - Test with missing dependencies
   - Test load/unload cycles

2. **Document Properly**
   - Complete inline documentation
   - Create user documentation
   - Include examples

3. **Submit for Review**
   - Create pull request
   - Include test results
   - Describe use cases

4. **Maintain Your Module**
   - Monitor for issues
   - Update for compatibility
   - Improve based on feedback

## Troubleshooting

### Module Won't Load
1. Check syntax: `bash -n bash_modules.d/mymodule.module`
2. Check dependencies: `bash_modules check-deps mymodule`
3. Check permissions: `ls -la bash_modules.d/mymodule.module`
4. Enable debug logging: `MODULE_DEBUG=1 bash_modules load mymodule`

### Performance Issues
1. Profile the module: `time _init_mymodule`
2. Check for blocking operations
3. Move expensive operations to lazy loading
4. Cache computed values

### Conflicts with Other Modules
1. Use unique function names
2. Check for alias conflicts
3. Declare dependencies properly
4. Use module namespaces consistently

Remember: A good module is focused, fast, and friendly!