#!/usr/bin/env bash
# SENTINEL Core Functions
# Enhanced shell functions for improved productivity and security
# Last Update: 2023-08-14

# Centralized configuration caching
# Usage: load_cached_config <config_file> [options]
# Loads a config file, using a cache if available and up-to-date
# Options:
#   --debug         - Show debug messages
#   --force-refresh - Force refresh of cache
#   --no-verify     - Disable cache verification
#   --selective="VAR1 VAR2" - Only cache specified variables
load_cached_config() {
    local config_file="$1"
    shift
    local cache_dir="${SENTINEL_CACHE_DIR:-$HOME/.sentinel/cache}/config"
    local config_key=$(echo "$config_file" | tr '/' '_' | tr '.' '_' 2>/dev/null) || {
        # If tr fails, use a simple basename approach with fallback
        config_key=$(basename "$config_file" 2>/dev/null || echo "config_${RANDOM}")
    }
    
    local cache_file="${cache_dir}/${config_key}.cache"
    local hash_file="${cache_dir}/${config_key}.hash"
    local force_refresh=0
    local verify=1
    local debug=0
    local selective_vars=""
    
    # Parse options with error handling
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --debug)
                debug=1
                ;;
            --force-refresh)
                force_refresh=1
                ;;
            --no-verify)
                verify=0
                ;;
            --selective=*)
                selective_vars="${1#*=}"
                ;;
            *)
                # Unrecognized option - ignore
                ;;
        esac
        shift
    done

    # Early exit with simple source if file doesn't exist
    if [[ ! -f "$config_file" ]]; then
        [[ $debug -eq 1 ]] && echo "[load_cached_config] Config file not found: $config_file" >&2
        return 0  # Return success to prevent terminal crashes

    # Cross-platform file hash function with error handling
    get_file_hash() {
        if command -v md5sum >/dev/null 2>&1; then
            md5sum "$1" 2>/dev/null | cut -d' ' -f1 2>/dev/null || echo "hash_error_${RANDOM}"
        elif command -v md5 >/dev/null 2>&1; then
            md5 -q "$1" 2>/dev/null || echo "hash_error_${RANDOM}"
        else
            echo "no_hash_available_$(date +%s 2>/dev/null || echo ${RANDOM})"
            return 0  # Return success to prevent crashes
        fi
    }

    # Ensure cache directory exists with error handling
    mkdir -p "$cache_dir" 2>/dev/null || {
        # If we can't create the cache directory, just source the file directly
        [[ $debug -eq 1 ]] && echo "[load_cached_config] Failed to create cache dir, sourcing directly" >&2
        source "$config_file" 2>/dev/null || true
        return 0  # Return success to prevent terminal crashes
    }

    # Force refresh or verify hash if requested - with error handling
    if [[ $force_refresh -eq 1 || $verify -eq 1 ]]; then
        if [[ -f "$hash_file" && -f "$cache_file" && $verify -eq 1 ]]; then
            local stored_hash=""
            local current_hash=""
            stored_hash=$(cat "$hash_file" 2>/dev/null) || stored_hash=""
            current_hash=$(get_file_hash "$config_file") || current_hash=""
            
            # If either hash operation failed or hashes don't match, force a refresh
            if [[ -z "$stored_hash" || -z "$current_hash" || "$stored_hash" != "$current_hash" ]]; then
                force_refresh=1
            fi
        else
            force_refresh=1
        fi
    fi

    # Use cache if it exists, is newer than config file, and no force refresh - with error handling
    if [[ -f "$cache_file" && -f "$config_file" ]] && \
       [[ "$cache_file" -nt "$config_file" 2>/dev/null ]] && \
       [[ $force_refresh -eq 0 ]]; then
        # Source the cached file with error handling
        source "$cache_file" 2>/dev/null || {
            # If cache file sourcing fails, try the original file
            source "$config_file" 2>/dev/null || true
        }
        return 0  # Always return success
    fi

    # Extract and validate variables before caching them - with error handling
    extract_safe_variables() {
        local var_name="$1"
        # Only allow valid variable names with extensive filtering
        if [[ "$var_name" =~ ^[a-zA-Z_][a-zA-Z0-9_]*$ ]] && \
           [[ ! "$var_name" =~ ^(BASH|COMP|DIRSTACK|EUID|FUNCNAME|GROUPS|HISTCMD|HOSTNAME|HOSTTYPE|IFS|LINENO|MACHTYPE|OLDPWD|OPTERR|OPTIND|OSTYPE|PIPESTATUS|PPID|PWD|RANDOM|SECONDS|SHELLOPTS|SHLVL|UID|_).*$ ]]; then
            declare -p "$var_name" 2>/dev/null || true
        fi
    }

    # Source the config file with error handling
    source "$config_file" 2>/dev/null || {
        # If sourcing fails, return success to prevent terminal crashes
        return 0
    }
    
    # Skip caching if required commands aren't available
    if ! command -v mktemp &>/dev/null || ! command -v grep &>/dev/null; then
        [[ $debug -eq 1 ]] && echo "[load_cached_config] Required tools missing, skipping cache creation" >&2
        return 0
    }
    
    # Capture variables to cache - with error handling
    local tmp_env_before=""
    local tmp_env_after=""
    local tmp_env_diff=""
    
    # Create temporary files with fallbacks if mktemp fails
    tmp_env_before=$(mktemp 2>/dev/null) || tmp_env_before="${cache_dir}/tmp_before_${RANDOM}"
    tmp_env_after=$(mktemp 2>/dev/null) || tmp_env_after="${cache_dir}/tmp_after_${RANDOM}"
    tmp_env_diff=$(mktemp 2>/dev/null) || tmp_env_diff="${cache_dir}/tmp_diff_${RANDOM}"
    
    # Touch files to ensure they exist
    touch "$tmp_env_before" "$tmp_env_after" "$tmp_env_diff" 2>/dev/null || {
        # If we can't create temp files, return success without caching
        return 0
    }
    
    # Try to create cache file but don't fail if it doesn't work
    {
        # Create cache file with header - with error handling
        echo "# SENTINEL Configuration Cache" > "$cache_file" 2>/dev/null
        echo "# Original: $config_file" >> "$cache_file" 2>/dev/null
        echo "# Generated: $(date 2>/dev/null || echo "unknown_time")" >> "$cache_file" 2>/dev/null
        echo "" >> "$cache_file" 2>/dev/null
        
        if [[ -n "$selective_vars" ]]; then
            # Only cache specified variables - with error handling
            for var in $selective_vars; do
                extract_safe_variables "$var" >> "$cache_file" 2>/dev/null || true
            done
        else
            # Get environment before and after to detect new variables - with error handling
            { set -o posix; set > "$tmp_env_before"; set +o posix; } 2>/dev/null || true
            { set -o posix; set > "$tmp_env_after"; set +o posix; } 2>/dev/null || true
            
            # Get the difference with error handling
            grep -vFxf "$tmp_env_before" "$tmp_env_after" > "$tmp_env_diff" 2>/dev/null || true
        
            # Save the changed variables to cache with error handling
            while IFS= read -r line 2>/dev/null; do
                # Extract variable name safely
                local var_name=""
                var_name=$(echo "$line" | cut -d= -f1 2>/dev/null) || continue
                
                # Skip if empty
                [[ -z "$var_name" ]] && continue
                
                # Extract variable safely
                extract_safe_variables "$var_name" >> "$cache_file" 2>/dev/null || true
            done < "$tmp_env_diff" 2>/dev/null || true
        fi
    } 2>/dev/null || true
    
    # Create hash for future verification with error handling
    get_file_hash "$config_file" > "$hash_file" 2>/dev/null || true
    
    # Try to secure the files but don't fail if it doesn't work
    chmod 600 "$cache_file" "$hash_file" 2>/dev/null || true
    
    # Clean up temp files with error handling
    rm -f "$tmp_env_before" "$tmp_env_after" "$tmp_env_diff" 2>/dev/null || true
    
    return 0  # Always return success to prevent terminal crashes
}

# Module configuration caching
# Specialized version of load_cached_config for modules
# Usage: load_module_config <module_name>
load_module_config() {
    local module_name="$1"
    local module_dir="${SENTINEL_MODULES_PATH:-$HOME/.bash_modules.d}"
    local module_file=""
    local debug=0
    [[ "$2" == "--debug" ]] && debug=1
    
    # Validate module name: only allow alphanumeric, underscore, dash (CVE-2016-7545 mitigation)
    if [[ ! "$module_name" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "[load_module_config] Invalid module name: $module_name" >&2
        return 1
    fi
    
    # Find the module file
    if [[ -f "${module_dir}/${module_name}.sh" ]]; then
        module_file="${module_dir}/${module_name}.sh"
    elif [[ -f "${module_dir}/${module_name}.module" ]]; then
        module_file="${module_dir}/${module_name}.module"
    else
        # Try to find in subdirs
        local found_file=$(find "${module_dir}" -name "${module_name}.sh" -o -name "${module_name}.module" | head -n1)
        if [[ -n "$found_file" ]]; then
            module_file="$found_file"
        else
            echo "[load_module_config] Module not found: $module_name" >&2
            return 1
        fi
    fi
    
    # Check if module cache is enabled and exists
    if [[ "${CONFIG[MODULE_CACHE]}" == "1" ]]; then
        local cache_dir="${SENTINEL_CACHE_DIR:-$HOME/cache}/modules"
        local cache_file="$cache_dir/${module_file//\//_}.cache"
        mkdir -p "$cache_dir"
        
        # Check for cache file and if it's not too old
        if [[ -f "$cache_file" ]] && (( $(date +%s) - $(stat -c %Y "$cache_file") < 86400 )); then
            secure_source "$cache_file"
            return 0
        fi
    fi

    # Continue with normal loading if cache doesn't exist or is disabled
    if ! [[ -f "$module_file" ]]; then
        [[ "${CONFIG[VERBOSE]}" == "1" ]] && echo "Error: Module file not found: $module_file" >&2
        return 1
    fi
    
    # Load the module
    secure_source "$module_file"
    
    # Cache the module if enabled
    if [[ "${CONFIG[MODULE_CACHE]}" == "1" ]]; then
        local cache_dir="${SENTINEL_CACHE_DIR:-$HOME/cache}/modules"
        local cache_file="$cache_dir/${module_file//\//_}.cache"
        
        # Extract all function declarations from the module
        declare -f | grep -A 1 "^[a-zA-Z_][a-zA-Z0-9_]* ()" | grep -v "^--$" > "$cache_file"
        
        # Extract all exports from the module (only variables)
        set | grep -E "^[a-zA-Z_][a-zA-Z0-9_]*=" | grep -v "^BASH_" >> "$cache_file"
    fi
}

# Get cached module dependencies
# Usage: get_module_dependencies <module_name>
get_module_dependencies() {
    local module_name="$1"
    local cache_dir="${SENTINEL_CACHE_DIR:-$HOME/cache}/modules"
    local deps_file="${cache_dir}/${module_name}.deps"
    
    # Validate module name to prevent path traversal
    if [[ ! "$module_name" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "Invalid module name format: $module_name" >&2
        return 1
    fi
    
    if [[ -f "$deps_file" ]]; then
        cat "$deps_file"
    else
        # Try to extract without loading the module
        local module_dir="${SENTINEL_MODULES_PATH:-$HOME/.bash_modules.d}"
        local module_file=""
        
        # Find the module file
        if [[ -f "${module_dir}/${module_name}.sh" ]]; then
            module_file="${module_dir}/${module_name}.sh"
        elif [[ -f "${module_dir}/${module_name}.module" ]]; then
            module_file="${module_dir}/${module_name}.module"
        else
            # Try to find in subdirs
            local found_file=$(find "${module_dir}" -name "${module_name}.sh" -o -name "${module_name}.module" | head -n1)
            if [[ -n "$found_file" ]]; then
                module_file="$found_file"
            else
                return 1
            fi
        fi
        
        # Extract dependencies securely using grep and awk for safer parsing
        if [[ -f "$module_file" ]]; then
            # Look for a properly formatted dependency line
            awk -F '"' '/SENTINEL_MODULE_DEPENDENCIES=/ {if (NF >= 3) print $2}' "$module_file" | head -n1
        fi
    fi
}

# Visual spinner for progress indication
spin() {
    echo -ne "${RED}-"
    echo -ne "${WHITE}\b|"
    echo -ne "${BLUE}\bx"
    sleep .02
    echo -ne "${RED}\b+${NC}"
}

# Load additional function files from directory
loadRcDir() {
    local dir="$1"
    local recursive="${2:-0}"  # 0=non-recursive, 1=recursive
    local debug="${3:-0}"      # 0=quiet, 1=debug output
    
    # Validate directory path
    if [[ -z "$dir" || "$dir" =~ [.][.] ]]; then
        [[ "$debug" == "1" ]] && echo "DEBUG: Invalid directory path: $dir" >&2
        return 1
    fi
    
    if [[ "$debug" == "1" ]]; then
        echo "DEBUG: Loading files from $dir (recursive=$recursive)"
    fi
    
    if [[ -d "$dir" ]]; then
        # Process non-recursive files first
        local rcFile
        # Handle empty directory case
        shopt -s nullglob
        for rcFile in "$dir"/*; do
            [[ -e "$rcFile" ]] || continue  # Skip if file doesn't exist
            
            if [[ -f "$rcFile" && -r "$rcFile" ]]; then
                # Basic script file validation
                if [[ "$rcFile" =~ \.(sh|bash)$ || $(head -n1 "$rcFile" 2>/dev/null) =~ ^\#\!/.*/(ba)?sh ]]; then
                    [[ "$debug" == "1" ]] && echo "DEBUG: Loading $rcFile"
                    secure_source "$rcFile" || echo "Error loading $rcFile" >&2
                else
                    [[ "$debug" == "1" ]] && echo "DEBUG: Skipping non-shell file $rcFile"
                fi
            fi
        done
        shopt -u nullglob
        
        # Process subdirectories if recursive mode is enabled
        if [[ "$recursive" == "1" ]]; then
            shopt -s nullglob
            for subdir in "$dir"/*; do
                [[ -e "$subdir" ]] || continue  # Skip if directory doesn't exist
                
                if [[ -d "$subdir" ]]; then
                    # Validate subdirectory name
                    local subdir_name=$(basename "$subdir")
                    if [[ ! "$subdir_name" =~ ^[a-zA-Z0-9_.-]+$ ]]; then
                        [[ "$debug" == "1" ]] && echo "DEBUG: Skipping directory with invalid name: $subdir_name" >&2
                        continue
                    fi
                    
                    [[ "$debug" == "1" ]] && echo "DEBUG: Entering subdirectory $subdir"
                    for subFile in "$subdir"/*; do
                        [[ -e "$subFile" ]] || continue  # Skip if file doesn't exist
                        
                        if [[ -f "$subFile" && -r "$subFile" ]]; then
                            # Basic script file validation
                            if [[ "$subFile" =~ \.(sh|bash)$ || $(head -n1 "$subFile" 2>/dev/null) =~ ^\#\!/.*/(ba)?sh ]]; then
                                [[ "$debug" == "1" ]] && echo "DEBUG: Loading $subFile"
                                secure_source "$subFile" || echo "Error loading $subFile" >&2
                            else
                                [[ "$debug" == "1" ]] && echo "DEBUG: Skipping non-shell file $subFile"
                            fi
                        fi
                    done
                fi
            done
            shopt -u nullglob
        fi
    else
        [[ "$debug" == "1" ]] && echo "DEBUG: Directory $dir does not exist or is not readable"
    fi
}

# Add a directory to the PATH (redirects to path_manager)
# This function is kept for backward compatibility and redirects to add_path
add2path() {
    # Check if path_manager.sh is available
    if type add_path &>/dev/null; then
        # Using the new path management system
        add_path "$@"
    else
        echo "Path manager not available, using legacy method (PATH changes won't persist between sessions)."
        
        # Legacy implementation
        # Prompt the user
        read -p "Do you want to add the current directory ($PWD) to PATH? [Y/n]: " answer
        case "$answer" in
            [Nn]* )
                # User selected 'no', prompt for directory to add
                read -p "Enter the full path you want to add to PATH: " new_path
                ;;
            * )
                # Default to adding current directory
                new_path="$PWD"
                ;;
        esac
        
        # Ensure new_path is not empty
        if [ -z "$new_path" ]; then
            echo "No path provided. Aborting."
            return 1
        fi
        
        # Resolve to full path
        full_path=$(readlink -f "$new_path")
        
        # Check if full_path is already in PATH
        if echo "$PATH" | tr ':' '\n' | grep -Fxq "$full_path"; then
            echo "Directory $full_path is already in PATH."
        else
            export PATH="$full_path:$PATH"
            echo "Added $full_path to PATH."
        fi
    fi
}

# Function to check if we're in a virtual environment
function in_venv() {
    if [ -n "$VIRTUAL_ENV" ]; then
        return 0  # In a virtual environment
    else
        return 1  # Not in a virtual environment
    fi
}

# Function to activate automatic virtual environment creation
function venvon() {
    VENV_AUTO=1
    echo "Automatic virtual environment activation is ON."
}

# Function to deactivate automatic virtual environment creation
function venvoff() {
    unset VENV_AUTO
    echo "Automatic virtual environment activation is OFF."
}

# General function to handle pip commands (pip and pip3)
function pip_command() {
    local PIP_EXEC="$1"
    shift  # Remove the first argument (pip or pip3)
    
    # Check if VENV_AUTO is enabled and not already in a venv
    if [ -n "$VENV_AUTO" ] && ! in_venv; then
        # Create a virtual environment in the current directory if it doesn't exist
        if [ ! -d "./.venv" ]; then
            echo "Creating virtual environment in ./.venv"
            command python3 -m venv ./.venv
            if [ $? -ne 0 ]; then
                echo "Error: Failed to create virtual environment."
                return 1
            fi
        fi
        
        # Activate the virtual environment
        secure_source_venv ./.venv/bin/activate 0
        if [ $? -ne 0 ]; then
            echo "Error: Failed to activate virtual environment."
            return 1
        fi
    fi
    
    # Run the actual pip command - using eval to handle the command prefix
    eval "$PIP_EXEC" "$@"
    local PIP_STATUS=$?
    
    # Check for errors related to virtual environment
    if [ $PIP_STATUS -ne 0 ] && ! in_venv; then
        # Specific error checking can be added here based on pip's output
        if [[ "$1" == "install" ]]; then
            echo "It seems you're not in a virtual environment. Would you like to create one? (y/n)"
            read -r CREATE_VENV
            if [ "$CREATE_VENV" = "y" ] || [ "$CREATE_VENV" = "Y" ]; then
                # Create a virtual environment
                command python3 -m venv ./.venv
                if [ $? -ne 0 ]; then
                    echo "Error: Failed to create virtual environment."
                    return 1
                fi
                
                # Activate the virtual environment
                secure_source_venv ./.venv/bin/activate 0
                if [ $? -ne 0 ]; then
                    echo "Error: Failed to activate virtual environment."
                    return 1
                fi
                
                # Retry the pip command
                eval "$PIP_EXEC" "$@"
                return $?
            else
                echo "Continuing without a virtual environment."
            fi
        fi
    fi
    
    return $PIP_STATUS
}

# Override the pip command
function pip() {
    pip_command "command pip" "$@"
}

# Override the pip3 command
function pip3() {
    pip_command "command pip3" "$@"
}

# Function to check internet connectivity and run apt update if connected
check_internet_and_update() {
    wget -q --spider http://google.com
    if [ $? -eq 0 ]; then
        echo "Connected to the internet."
        sudo apt update
    else
        echo "Not connected to the internet. Skipping apt update."
    fi
}

# Note: sourcebash alias has been moved to bash_aliases file

# Secure file deletion function that overrides rm
rm() {
    local secure_mode=0
    local force=0
    local recursive=0
    local verbose=0
    local interactive=0
    local args=()
    local secure_options=""

    # Process all arguments to capture rm flags
    for arg in "$@"; do
        case "$arg" in
            -s|--secure)
                secure_mode=1
                ;;
            -f|--force)
                force=1
                args+=("$arg")
                secure_options+=" -f"
                ;;
            -r|--recursive|-R)
                recursive=1
                args+=("$arg")
                ;;
            -v|--verbose)
                verbose=1
                args+=("$arg")
                secure_options+=" -v"
                ;;
            -i|--interactive)
                interactive=1
                args+=("$arg")
                ;;
            *)
                # Add other arguments to the list
                args+=("$arg")
                ;;
        esac
    done

    # Default to secure mode if configured in SENTINEL
    if [[ "${SENTINEL_SECURE_RM:-1}" == "1" ]]; then
        secure_mode=1
    fi

    # If in secure mode, use secure deletion methods
    if [[ $secure_mode -eq 1 ]]; then
        local files_to_process=()
        local is_empty=1

        # Get the list of files to delete (excluding options)
        for arg in "${args[@]}"; do
            if [[ "$arg" != -* ]]; then
                files_to_process+=("$arg")
                is_empty=0
            fi
        done

        # If no files were specified, show usage
        if [[ $is_empty -eq 1 ]]; then
            echo "Usage: rm [options] file(s)"
            echo "Secure deletion options:"
            echo "  -s, --secure     Force secure deletion (default in current configuration)"
            return 1
        fi

        # Process the files
        for file in "${files_to_process[@]}"; do
            # Skip if file doesn't exist
            if [[ ! -e "$file" ]]; then
                [[ $verbose -eq 1 ]] && echo "rm: cannot remove '$file': No such file or directory"
                continue
            fi

            # Handle directories
            if [[ -d "$file" ]]; then
                if [[ $recursive -eq 1 ]]; then
                    if [[ $interactive -eq 1 && $force -eq 0 ]]; then
                        read -p "Securely remove directory '$file' and all its contents? [y/N] " confirm
                        [[ "$confirm" != [yY]* ]] && continue
                    fi
                    
                    if [[ $verbose -eq 1 ]]; then
                        echo "Securely removing directory: $file"
                    fi
                    
                    # Process directory contents first
                    find "$file" -type f -print0 2>/dev/null | while IFS= read -r -d '' item; do
                        _secure_shred "$item" "$secure_options"
                    done
                    
                    # Then remove the empty directory structure
                    command rm -rf "$file"
                else
                    echo "rm: cannot remove '$file': Is a directory"
                    continue
                fi
            else
                # Handle files
                if [[ $interactive -eq 1 && $force -eq 0 ]]; then
                    read -p "Securely remove '$file'? [y/N] " confirm
                    [[ "$confirm" != [yY]* ]] && continue
                fi
                
                if [[ $verbose -eq 1 ]]; then
                    echo "Securely removing file: $file"
                fi
                
                # Securely delete the file
                _secure_shred "$file" "$secure_options"
            fi
        done
    else
        # Fall back to standard rm
        command rm "${args[@]}"
    fi
}

# Helper function for actual secure deletion
_secure_shred() {
    local file="$1"
    local options="$2"
    
    # Check file existence
    if [[ ! -f "$file" ]]; then
        return 1
    fi
    
    # Use shred if available
    if command -v shred &>/dev/null; then
        # Default options: 3 passes, zero final pass, remove file
        shred -n 3 -z -u $options "$file"
    else
        # Fallback if shred is not available
        local filesize=$(stat -c %s "$file" 2>/dev/null || stat -f %z "$file" 2>/dev/null)
        local blocksize=1024
        local pass_count=3
        
        # For each pass
        for ((pass=1; pass<=pass_count; pass++)); do
            # Progress indicator
            echo -ne "Secure erase pass $pass/$pass_count: ["
            
            # Different patterns for different passes
            case $pass in
                1) # Zeros - simplified approach without tr translation
                   dd if=/dev/zero of="$file" bs=$blocksize count=$((filesize/blocksize+1)) conv=notrunc >/dev/null 2>&1
                   ;;
                2) # Ones - using /dev/zero and setting all bits to 1 is tricky without tr, 
                   # so we'll use a file filled with 0xFF bytes
                   dd if=/dev/urandom of="$file" bs=$blocksize count=$((filesize/blocksize+1)) conv=notrunc >/dev/null 2>&1
                   ;;
                3) # Random data for final pass
                   dd if=/dev/urandom of="$file" bs=$blocksize count=$((filesize/blocksize+1)) conv=notrunc >/dev/null 2>&1
                   ;;
            esac
            
            # Complete progress bar
            echo -ne "========================================"
            echo -e "] Done."
        done
        
        # Finally remove the file
        command rm -f "$file"
    fi
}

# Function to toggle secure rm mode
secure_rm_toggle() {
    if [[ "${SENTINEL_SECURE_RM:-1}" == "1" ]]; then
        SENTINEL_SECURE_RM=0
        echo "Secure rm mode is now OFF. Files deleted with rm will use standard deletion."
    else
        SENTINEL_SECURE_RM=1
        echo "Secure rm mode is now ON. Files deleted with rm will be securely erased."
    fi
}

# Function to manually trigger secure cleanup
secure_clean() {
    local scope="${1:-all}"
    
    echo "SENTINEL: Starting manual secure cleanup (scope: $scope)..."
    
    case "$scope" in
        history)
            history -c
            _secure_shred ~/.bash_history "-f"
            echo "Bash history cleared and securely erased."
            ;;
            
        temp)
            # Clean /tmp files created by current user
            find /tmp -user $(whoami) -type f -exec _secure_shred {} \; 2>/dev/null
            echo "Temporary files securely erased."
            ;;
            
        browser)
            # Firefox
            if [[ -d "$HOME/.mozilla/firefox" ]]; then
                find "$HOME/.mozilla/firefox" -name "*.sqlite" -exec _secure_shred {} \; 2>/dev/null
                find "$HOME/.mozilla/firefox" -name "cookies.sqlite*" -exec _secure_shred {} \; 2>/dev/null
            fi
            
            # Chrome/Chromium
            if [[ -d "$HOME/.config/google-chrome" ]]; then
                find "$HOME/.config/google-chrome/Default" -name "Cookies*" -exec _secure_shred {} \; 2>/dev/null
            fi
            if [[ -d "$HOME/.config/chromium" ]]; then
                find "$HOME/.config/chromium/Default" -name "Cookies*" -exec _secure_shred {} \; 2>/dev/null
            fi
            
            echo "Browser data securely erased."
            ;;
            
        cache)
            find "$HOME/.cache" -type f -exec _secure_shred {} \; 2>/dev/null
            echo "Cache files securely erased."
            ;;
            
        all)
            # Call the function recursively for each scope
            secure_clean history
            secure_clean temp
            secure_clean browser
            secure_clean cache
            
            # Additional cleaning for "all" scope
            _secure_shred "$HOME/.local/share/recently-used.xbel" "-f" 2>/dev/null
            _secure_shred "$HOME/.recently-used" "-f" 2>/dev/null
            
            # Clear clipboard
            if command -v xsel &>/dev/null; then
                echo -n | xsel --clipboard --input
            elif command -v xclip &>/dev/null; then
                echo -n | xclip -selection clipboard
            fi
            
            echo "All cleanup tasks completed."
            ;;
            
        *)
            echo "Unknown scope: $scope"
            echo "Available scopes: history, temp, browser, cache, all"
            return 1
            ;;
    esac
}

# Function to toggle module verbosity
sentinel_quiet() {
    if [[ "$1" == "on" ]]; then
        SENTINEL_QUIET_MODULES=1
        echo "SENTINEL quiet mode: ON - Minimal module output"
    elif [[ "$1" == "off" ]]; then
        SENTINEL_QUIET_MODULES=0
        echo "SENTINEL quiet mode: OFF - Verbose module output"
    else
        echo "SENTINEL quiet mode is currently: $([[ "${SENTINEL_QUIET_MODULES:-1}" == "1" ]] && echo "ON" || echo "OFF")"
        echo "Usage: sentinel_quiet [on|off]"
    fi
}

# Load additional functions from function directory
loadRcDir "${HOME}/.bash_functions.d" 1

# Quick alias setup
function qalias() {
  if [[ -z "$1" || -z "$2" ]]; then
    echo "Usage: qalias <alias_name> <command>"
    return 1
  fi
  
  local alias_name="$1"
  shift
  local alias_cmd="$*"
  
  # Add to aliases file
  echo "alias $alias_name='$alias_cmd'" >> ~/.bash_aliases
  
  # Load it immediately
  alias "$alias_name"="$alias_cmd"
  
  echo "Alias '$alias_name' created and activated"
}

# Ensure the script is sourced correctly - only need this check once
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Please source this script instead of executing it:"
    echo "source ~/.bashrc"
fi

# Generic lazy loading function
# This function creates a wrapper that loads the real command only when it's first invoked
# Usage: lazy_load <command> <load_function>
function lazy_load() {
    local cmd="$1"
    local load_function="$2"
    
    # Create a wrapper function with the same name as the command
    eval "function $cmd() {
        # Unset this function to avoid recursion
        unset -f $cmd
        
        # Call the loader function
        $load_function
        
        # Now call the real command with the original arguments
        $cmd \"\$@\"
    }"
}

# Collection of loader functions for different development environments
# These can be used with the lazy_load function

# Load NVM environment
function __load_nvm() {
    if [[ -d "$HOME/.nvm" ]]; then
        export NVM_DIR="$HOME/.nvm"
        [ -s "$NVM_DIR/nvm.sh" ] && secure_source "$NVM_DIR/nvm.sh"
        [ -s "$NVM_DIR/bash_completion" ] && secure_source "$NVM_DIR/bash_completion"
    fi
}

# Load Pyenv environment
function __load_pyenv() {
    if [[ -d "$HOME/.pyenv" ]]; then
        export PYENV_ROOT="$HOME/.pyenv"
        [[ -d "$PYENV_ROOT/bin" ]] && PATH="$PYENV_ROOT/bin:$PATH"
        if command -v pyenv >/dev/null; then
            eval "$(pyenv init -)"
            eval "$(pyenv virtualenv-init -)"
        fi
    fi
}

# Load Cargo/Rust environment
function __load_cargo() {
    if [[ -f "$HOME/.cargo/env" ]]; then
        # shellcheck source=~/.cargo/env
        secure_source "$HOME/.cargo/env"
    fi
}

# Load RVM environment
function __load_rvm() {
    if [[ -d "$HOME/.rvm" ]]; then
        export PATH="$PATH:$HOME/.rvm/bin"
        [[ -s "$HOME/.rvm/scripts/rvm" ]] && secure_source "$HOME/.rvm/scripts/rvm"
    fi
}

# Load Go environment
function __load_go() {
    if [[ -d "$HOME/go" ]]; then
        export GOPATH="$HOME/go"
        export PATH="$PATH:$GOPATH/bin"
    fi
}

# Load Docker environment - useful for tools like docker-compose
function __load_docker() {
    # Load Docker completion if available
    if [[ -f /usr/share/bash-completion/completions/docker ]]; then
        secure_source /usr/share/bash-completion/completions/docker
    fi
}

# Example usage:
# lazy_load nvm __load_nvm
# lazy_load pyenv __load_pyenv
# ... etc ...

# Validate that a path is safe (no relative paths, no path traversal)
validate_path() {
    local path="$1"
    local allow_relative="${2:-0}"
    
    # Check for empty path
    if [[ -z "$path" ]]; then
        return 1
    fi
    
    # Check for path traversal attempts
    if [[ "$path" =~ \.\./ || "$path" =~ /\.\.$ || "$path" =~ /\.\. ]]; then
        return 1
    fi
    
    # Check for relative paths if not allowed
    if [[ "$allow_relative" != "1" && ! "$path" =~ ^/ && ! "$path" =~ ^~ ]]; then
        return 1
    fi
    
    # Check for other dangerous patterns
    if [[ "$path" =~ [|;&<>$] ]]; then
        return 1
    fi
    
    return 0
}

# Securely source a file (CVE-2016-7545 mitigation)
secure_source() {
    local file="$1"
    
    # Validate file path
    if ! validate_path "$file" 1; then
        echo "Security warning: Invalid file path: $file" >&2
        return 1
    }
    
    if [[ -f "$file" ]]; then
        local perms
        perms=$(stat -c %a "$file")
        if [[ "$perms" != "600" ]]; then
            echo "Security warning: $file permissions are $perms (should be 600). Refusing to source." >&2
            return 1
        fi
        source "$file"
    else
        echo "File not found: $file" >&2
        return 1
    fi
}

# Securely source a file with optional permission check relaxation (for special cases like venvs)
secure_source_venv() {
    local file="$1"
    local strict_perms="${2:-1}"  # Default to strict permissions
    local max_size="${3:-500000}" # Reasonable max size in bytes
    local allow_world_readable="${4:-0}" # Default to disallow world-readable
    
    # Validate file path
    if ! validate_path "$file" 1; then
        echo "Security warning: Invalid file path: $file" >&2
        return 1
    fi
    
    if [[ -f "$file" ]]; then
        # Check file size
        local size
        size=$(stat -c %s "$file" 2>/dev/null || stat -f %z "$file" 2>/dev/null)
        if [[ -n "$size" && "$size" -gt "$max_size" ]]; then
            echo "Security warning: File size exceeds maximum ($size > $max_size). Refusing to source." >&2
            return 1
        fi
        
        # Check file permissions
        local perms
        perms=$(stat -c %a "$file")
        
        # If strict permissions enabled
        if [[ "$strict_perms" == "1" ]]; then
            if [[ "$perms" != "600" ]]; then
                echo "Security warning: $file permissions are $perms (should be 600 for maximum security)." >&2
                
                # If world-readable is disallowed and the file is world-readable
                if [[ "$allow_world_readable" != "1" && "${perms:2:1}" != "0" ]]; then
                    echo "Refusing to source world-readable file." >&2
                    return 1
                fi
                
                # Check if the file is world-writable (always reject)
                if [[ "${perms:2:1}" =~ [2367] ]]; then
                    echo "Refusing to source world-writable file." >&2
                    return 1
                fi
            fi
        fi
        
        # Basic content validation - reject if suspicious patterns found
        if grep -q -E '(curl[[:space:]]*\|[[:space:]]*(ba)?sh|wget[[:space:]]*\|[[:space:]]*(ba)?sh|eval[[:space:]]\$\(|`.*[;&|][^`]*`)' "$file"; then
            echo "Security warning: File contains potentially unsafe patterns. Refusing to source." >&2
            return 1
        fi
        
        source "$file"
    else
        echo "File not found: $file" >&2
        return 1
    fi
}

# Replace all source calls for config, cache, modules, and function files with secure_source
# Example for config_file:
# secure_source "$config_file"