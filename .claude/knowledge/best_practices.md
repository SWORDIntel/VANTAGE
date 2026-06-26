# VANTAGE Best Practices

## Overview

This document contains established best practices learned from developing and maintaining VANTAGE. Follow these guidelines to ensure code quality, performance, and maintainability.

## General Principles

### 1. Performance First
Bash startup time is critical. Every millisecond counts.

```bash
# BAD: Loading everything upfront
source heavy_module.sh
source another_heavy_module.sh
initialize_everything

# GOOD: Lazy loading
alias heavy_command='source heavy_module.sh && heavy_command'
```

### 2. Fail Gracefully
Never break the user's shell, even when things go wrong.

```bash
# BAD: Exits on error
command_that_might_fail || exit 1

# GOOD: Handle errors gracefully
if ! command_that_might_fail 2>/dev/null; then
    log_warning "Feature X unavailable: command failed"
    return 0  # Don't break the shell
fi
```

### 3. Defensive Programming
Always assume things can go wrong.

```bash
# BAD: Assumes variable is set
process_data "$USER_INPUT"

# GOOD: Validates before use
if [[ -n "${USER_INPUT:-}" ]]; then
    process_data "$USER_INPUT"
else
    log_error "No input provided"
    return 1
fi
```

## Bash-Specific Best Practices

### 1. Use Modern Bash Features
VANTAGE requires Bash 4.0+, so use its features.

```bash
# Associative arrays
declare -A config_map
config_map[key]="value"

# Process substitution
while read -r line; do
    process_line "$line"
done < <(command_that_outputs_lines)

# Better string manipulation
string="hello:world"
echo "${string%%:*}"  # hello
echo "${string#*:}"   # world
```

### 2. Quote Everything
Prevent word splitting and globbing issues.

```bash
# BAD
file_path=$HOME/my documents/file.txt
cat $file_path

# GOOD
file_path="$HOME/my documents/file.txt"
cat "$file_path"

# BETTER: Handle special cases
file_path="${HOME}/my documents/file.txt"
[[ -f "$file_path" ]] && cat "$file_path"
```

### 3. Use Local Variables
Prevent variable pollution in the global namespace.

```bash
# BAD
temp_var="something"
process_data() {
    result=$(echo "$temp_var" | tr '[:lower:]' '[:upper:]')
    echo "$result"
}

# GOOD
process_data() {
    local temp_var="something"
    local result
    result=$(echo "$temp_var" | tr '[:lower:]' '[:upper:]')
    echo "$result"
}
```

### 4. Check Command Availability
Don't assume commands exist.

```bash
# Function to check command availability
has_command() {
    command -v "$1" &>/dev/null
}

# Usage
if has_command fzf; then
    alias search='fzf'
else
    alias search='find . -name'
fi
```

## Module Development Best Practices

### 1. Module Structure
Keep modules focused and well-organized.

```bash
# Good module structure:
# 1. Metadata
# 2. Configuration
# 3. Initialization
# 4. Core functions
# 5. Helper functions
# 6. Cleanup
# 7. Self-test
```

### 2. Namespace Functions
Prevent naming conflicts.

```bash
# BAD: Generic names
process() { ... }
check() { ... }

# GOOD: Namespaced
mymodule_process() { ... }
mymodule_check() { ... }

# BETTER: Consistent prefix
_mymodule_init() { ... }
_mymodule_helper() { ... }
mymodule_public_function() { ... }
```

### 3. Handle Dependencies
Check and declare all dependencies.

```bash
# In module metadata
MODULE_DEPS=("config_cache" "logging")
MODULE_OPTIONAL_DEPS=("fzf" "python3")

# In initialization
_init_mymodule() {
    # Check required deps
    for dep in "${MODULE_DEPS[@]}"; do
        if ! is_module_loaded "$dep"; then
            log_error "Required dependency missing: $dep"
            return 1
        fi
    done
    
    # Check optional deps
    if has_command fzf; then
        MYMODULE_HAS_FZF=true
    fi
}
```

## Security Best Practices

### 1. Input Validation
Never trust user input.

```bash
# BAD: Direct use of input
eval "$USER_COMMAND"

# GOOD: Validate first
validate_command() {
    local cmd="$1"
    # Only allow alphanumeric and specific chars
    if [[ "$cmd" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        return 0
    fi
    return 1
}

if validate_command "$USER_COMMAND"; then
    # Safe to use
fi
```

### 2. Secure Defaults
Always default to the secure option.

```bash
# Module configuration
MODULE_CONFIG_VARS=(
    "MYMODULE_ALLOW_EXEC:false"      # Default: disabled
    "MYMODULE_VALIDATE_INPUT:true"   # Default: enabled
    "MYMODULE_LOG_COMMANDS:true"     # Default: enabled
)
```

### 3. Avoid Dangerous Constructs
Some bash features are inherently risky.

```bash
# AVOID: eval
eval "$command"

# BETTER: Use arrays and proper expansion
cmd_array=("ls" "-la" "$directory")
"${cmd_array[@]}"

# AVOID: Unquoted command substitution
result=`dangerous command`

# BETTER: Modern syntax with quotes
result="$(safe_command)"
```

## Performance Best Practices

### 1. Minimize Forking
External commands are expensive.

```bash
# BAD: Multiple external calls
file_count=$(ls | wc -l)
file_list=$(ls)
first_file=$(ls | head -1)

# GOOD: Single operation
files=(*)
file_count=${#files[@]}
first_file=${files[0]:-}
```

### 2. Cache Expensive Operations
Don't repeat costly calculations.

```bash
# Cache function results
declare -A _cache

cached_expensive_operation() {
    local key="$1"
    
    # Check cache first
    if [[ -n "${_cache[$key]:-}" ]]; then
        echo "${_cache[$key]}"
        return 0
    fi
    
    # Perform operation
    local result
    result=$(expensive_operation "$key")
    
    # Cache and return
    _cache[$key]="$result"
    echo "$result"
}
```

### 3. Use Bash Built-ins
Prefer bash built-ins over external commands.

```bash
# BAD: External commands
if [ $(echo "$string" | grep -c "pattern") -gt 0 ]; then

# GOOD: Bash pattern matching
if [[ "$string" == *"pattern"* ]]; then

# BAD: External basename
filename=$(basename "$path")

# GOOD: Bash parameter expansion
filename="${path##*/}"
```

## Error Handling Best Practices

### 1. Comprehensive Error Handling
Handle all failure modes.

```bash
safe_file_operation() {
    local file="$1"
    
    # Check file exists
    if [[ ! -f "$file" ]]; then
        log_error "File not found: $file"
        return 1
    fi
    
    # Check readable
    if [[ ! -r "$file" ]]; then
        log_error "File not readable: $file"
        return 2
    fi
    
    # Perform operation with error handling
    if ! process_file "$file" 2>/dev/null; then
        log_error "Failed to process file: $file"
        return 3
    fi
    
    return 0
}
```

### 2. Meaningful Error Messages
Help users understand and fix issues.

```bash
# BAD: Generic error
echo "Error!"

# GOOD: Specific and actionable
echo "Error: Configuration file not found at ~/.config/vantage/config.yml"
echo "Please run 'vantage config --init' to create a default configuration."
```

## Testing Best Practices

### 1. Test Everything
Every module should have tests.

```bash
# Module self-test function
_test_mymodule() {
    local errors=0
    
    # Test initialization
    echo -n "Testing initialization... "
    if [[ "${MYMODULE_LOADED:-}" == "1" ]]; then
        echo "PASS"
    else
        echo "FAIL"
        ((errors++))
    fi
    
    # Test core functionality
    echo -n "Testing core function... "
    if mymodule_function "test" &>/dev/null; then
        echo "PASS"
    else
        echo "FAIL"
        ((errors++))
    fi
    
    return $errors
}
```

### 2. Test Edge Cases
Don't just test the happy path.

```bash
test_string_function() {
    # Normal case
    assert_equals "expected" "$(string_function "input")"
    
    # Edge cases
    assert_equals "" "$(string_function "")"              # Empty string
    assert_equals "..." "$(string_function "   ")"        # Whitespace
    assert_equals "..." "$(string_function $'\n\t')"      # Special chars
    assert_equals "..." "$(string_function "very long")"  # Long input
}
```

## Documentation Best Practices

### 1. Document Intent, Not Just Function
Explain why, not just what.

```bash
# BAD: States the obvious
# This function processes data
process_data() { ... }

# GOOD: Explains purpose and context
# Normalizes user input data for storage in the cache.
# Handles special characters and enforces size limits to
# prevent cache pollution. Used by the autocomplete system.
process_data() { ... }
```

### 2. Provide Examples
Show, don't just tell.

```bash
# Function: find_large_files
# Purpose: Find files larger than specified size
# Usage: find_large_files [size] [directory]
# Examples:
#   find_large_files 100M /home      # Files > 100MB in /home
#   find_large_files 1G              # Files > 1GB in current dir
#   find_large_files 500K /tmp       # Files > 500KB in /tmp
find_large_files() { ... }
```

## Common Pitfalls to Avoid

### 1. Global Variable Pollution
```bash
# BAD: Pollutes global namespace
TEMP="/tmp/file"
RESULT="success"

# GOOD: Use function-local variables or namespace
_mymodule_temp="/tmp/file"
declare -g MYMODULE_RESULT="success"
```

### 2. Assuming Environment
```bash
# BAD: Assumes GNU tools
ls --color=auto

# GOOD: Check capabilities
if ls --color=auto &>/dev/null; then
    alias ls='ls --color=auto'
else
    # BSD/macOS compatibility
    alias ls='ls -G'
fi
```

### 3. Breaking User's Environment
```bash
# BAD: Overrides without checking
alias ls='my_custom_ls'
export PATH="/my/path:$PATH"

# GOOD: Preserve user preferences
alias ls="${USER_LS_ALIAS:-ls --color=auto}"
export PATH="${VANTAGE_BIN}:${PATH}"
```

## Maintenance Best Practices

### 1. Version Everything
Track versions for debugging and compatibility.

```bash
MODULE_VERSION="1.2.3"
MODULE_MIN_BASH_VERSION="4.0"
MODULE_API_VERSION="2"
```

### 2. Deprecate Gracefully
Don't break existing setups.

```bash
# Old function name (deprecated)
old_function_name() {
    log_warning "old_function_name is deprecated, use new_function_name"
    new_function_name "$@"
}
```

### 3. Log Appropriately
Use consistent logging levels.

```bash
log_debug "Detailed information for debugging"
log_info "Normal operational messages"
log_warning "Warning but not critical"
log_error "Error that needs attention"
log_critical "Critical error, feature disabled"
```

Remember: These practices have been learned through experience. Following them will save time and prevent issues!