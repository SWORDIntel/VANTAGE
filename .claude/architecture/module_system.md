# VANTAGE Module System Architecture

## Overview

The VANTAGE module system is a sophisticated dynamic loading framework that allows features to be added, removed, or modified without affecting the core system.

## Module Structure

### Basic Module Template

```bash
#!/bin/bash
# Module: example_module
# Version: 1.0.0
# Description: Example module showing structure
# Dependencies: none
# Author: VANTAGE Team

# Module metadata
MODULE_NAME="example_module"
MODULE_VERSION="1.0.0"
MODULE_DEPS=()
MODULE_DESCRIPTION="Example module for documentation"

# Module initialization
_init_example_module() {
    # Initialization code here
    export EXAMPLE_MODULE_LOADED=1
    return 0
}

# Module functions
example_function() {
    echo "This is an example function"
}

# Module cleanup (optional)
_cleanup_example_module() {
    unset EXAMPLE_MODULE_LOADED
}

# Auto-initialize if sourced directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    _init_example_module
fi
```

## Module Loading Process

### 1. Discovery Phase
```bash
# The module loader scans bash_modules.d/
for module_file in "$MODULE_DIR"/*.module; do
    # Extract module metadata
    MODULE_NAME=$(extract_metadata "MODULE_NAME" "$module_file")
    MODULE_DEPS=$(extract_metadata "MODULE_DEPS" "$module_file")
done
```

### 2. Dependency Resolution
```bash
# Topological sort of modules based on dependencies
resolve_dependencies() {
    local module=$1
    local -a deps=($(get_module_deps "$module"))
    
    for dep in "${deps[@]}"; do
        if ! is_loaded "$dep"; then
            load_module "$dep"
        fi
    done
}
```

### 3. Verification Phase
```bash
# HMAC verification (if enabled)
verify_module_integrity() {
    local module_file=$1
    local stored_hmac=$(get_stored_hmac "$module_file")
    local calculated_hmac=$(calculate_hmac "$module_file")
    
    [[ "$stored_hmac" == "$calculated_hmac" ]]
}
```

### 4. Loading Phase
```bash
# Actual module loading
load_module() {
    local module=$1
    
    # Pre-load hook
    run_hook "pre_load_${module}"
    
    # Source the module
    source "${MODULE_DIR}/${module}.module"
    
    # Initialize the module
    "_init_${module}"
    
    # Post-load hook
    run_hook "post_load_${module}"
    
    # Mark as loaded
    LOADED_MODULES+=("$module")
}
```

## Module Types

### 1. Core Modules
Essential system functionality
```bash
# Examples:
- config_cache.module    # Configuration caching
- hmac.module            # Security verification
- logging.module         # System logging
```

### 2. Enhancement Modules
Improve user experience
```bash
# Examples:
- autocomplete.module    # Tab completion
- fuzzy_correction.module # Command correction
- fzf.module             # Fuzzy finding
```

### 3. Tool Modules
Integrate external tools
```bash
# Examples:
- hashcat.module         # Password cracking
- distcc.module          # Distributed compilation
- vantage_chat.module   # AI chat interface
```

### 4. Security Modules
Security hardening features
```bash
# Examples:
- shell_security.module  # Shell hardening
- obfuscate.module       # Output obfuscation
- audit.module           # Command auditing
```

## Module Dependencies

### Declaring Dependencies
```bash
# In module file:
MODULE_DEPS=("logging" "config_cache")

# Optional dependencies:
MODULE_OPTIONAL_DEPS=("fzf" "python3")
```

### Dependency Types

1. **Hard Dependencies**: Module won't load without them
2. **Soft Dependencies**: Module loads but some features disabled
3. **Runtime Dependencies**: Checked when features are used

### Circular Dependency Prevention
```bash
# The loader maintains a dependency graph
DEPENDENCY_GRAPH[module]="dep1,dep2,dep3"

# Detect cycles using DFS
detect_circular_deps() {
    local visited=()
    local rec_stack=()
    # ... cycle detection algorithm
}
```

## Module Configuration

### Configuration Sources
1. Module defaults (in module file)
2. System configuration (/etc/vantage/)
3. User configuration (~/.config/vantage/)
4. Environment variables
5. Runtime configuration

### Configuration Example
```bash
# In module:
MODULE_CONFIG_VARS=(
    "EXAMPLE_TIMEOUT:30"      # var:default_value
    "EXAMPLE_ENABLED:true"
    "EXAMPLE_LOG_LEVEL:info"
)

# Loading configuration:
load_module_config() {
    for var_def in "${MODULE_CONFIG_VARS[@]}"; do
        var_name="${var_def%%:*}"
        default_val="${var_def#*:}"
        
        # Check sources in order
        value=$(get_config_value "$var_name" "$default_val")
        export "$var_name=$value"
    done
}
```

## Module API

### Standard Functions
Every module should implement:
```bash
_init_<module_name>()      # Initialize module
_cleanup_<module_name>()   # Cleanup (optional)
_config_<module_name>()    # Configuration UI (optional)
_help_<module_name>()      # Help text (optional)
```

### Hook System
```bash
# Available hooks:
pre_load_<module>          # Before loading
post_load_<module>         # After loading
pre_unload_<module>        # Before unloading
post_unload_<module>       # After unloading
```

### Event System
```bash
# Modules can register for events:
register_event_handler "command_not_found" "my_handler_func"
register_event_handler "prompt_display" "my_prompt_func"

# Trigger events:
trigger_event "command_not_found" "$command"
```

## Module Management

### Loading Modules
```bash
# Manual loading:
load_module "module_name"

# Conditional loading:
[[ -f "$MODULE_DIR/module_name.module" ]] && load_module "module_name"

# Lazy loading:
module_lazy_load "expensive_module" "trigger_command"
```

### Unloading Modules
```bash
unload_module() {
    local module=$1
    
    # Call cleanup function
    "_cleanup_${module}" 2>/dev/null
    
    # Remove from loaded list
    LOADED_MODULES=("${LOADED_MODULES[@]/$module}")
    
    # Unset module variables
    unset "MODULE_${module}_LOADED"
}
```

### Module Status
```bash
# Check if loaded:
is_module_loaded "module_name"

# List loaded modules:
list_loaded_modules

# Get module info:
get_module_info "module_name"
```

## Performance Optimization

### Lazy Loading
```bash
# Define trigger conditions:
MODULE_LAZY_TRIGGERS[git_enhanced]="git"
MODULE_LAZY_TRIGGERS[docker_tools]="docker"

# Intercept commands:
command_not_found_handler() {
    local cmd=$1
    for module in "${!MODULE_LAZY_TRIGGERS[@]}"; do
        if [[ "${MODULE_LAZY_TRIGGERS[$module]}" == "$cmd" ]]; then
            load_module "$module"
            "$@"  # Retry command
            return $?
        fi
    done
}
```

### Caching
```bash
# Cache module metadata:
cache_module_metadata() {
    declare -A MODULE_CACHE
    MODULE_CACHE[name]="$MODULE_NAME"
    MODULE_CACHE[version]="$MODULE_VERSION"
    MODULE_CACHE[deps]="${MODULE_DEPS[*]}"
    
    # Serialize to cache file
    declare -p MODULE_CACHE > "$CACHE_DIR/${MODULE_NAME}.cache"
}
```

### Parallel Loading
```bash
# Load independent modules in parallel:
parallel_load_modules() {
    local -a independent_modules=()
    
    # Find modules with no dependencies
    for module in "${ALL_MODULES[@]}"; do
        if [[ -z "${MODULE_DEPS[$module]}" ]]; then
            independent_modules+=("$module")
        fi
    done
    
    # Load in background
    for module in "${independent_modules[@]}"; do
        load_module "$module" &
    done
    wait
}
```

## Security Considerations

### Module Verification
1. HMAC signatures for integrity
2. Permission checks (not writable by others)
3. Path validation (no symlinks outside module dir)
4. Code signing (future feature)

### Sandboxing
```bash
# Restricted module execution:
run_sandboxed_module() {
    local module=$1
    (
        # Restricted environment
        set -r
        PATH="/usr/bin:/bin"
        unset LD_PRELOAD
        
        # Load module
        source "$module"
    )
}
```

### Audit Trail
```bash
# Log module operations:
log_module_operation() {
    local operation=$1
    local module=$2
    local timestamp=$(date +%s)
    
    echo "$timestamp $operation $module $USER" >> "$MODULE_AUDIT_LOG"
}
```

## Best Practices

### Module Development
1. Keep modules focused (single responsibility)
2. Declare all dependencies explicitly
3. Use namespaced function names
4. Provide meaningful error messages
5. Include inline documentation
6. Test with missing dependencies

### Performance
1. Minimize initialization code
2. Use lazy loading for expensive operations
3. Cache computed values
4. Avoid blocking operations
5. Profile module load times

### Security
1. Validate all inputs
2. Use secure defaults
3. Avoid eval and similar constructs
4. Check permissions before operations
5. Log security-relevant events

### Compatibility
1. Test on multiple bash versions
2. Handle missing commands gracefully
3. Provide fallbacks for features
4. Check for GNU vs BSD utilities
5. Document system requirements