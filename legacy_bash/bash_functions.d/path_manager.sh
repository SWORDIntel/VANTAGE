#!/usr/bin/env bash
# SENTINEL Path Manager
# Persistent path management solution for SENTINEL framework
# Last Update: 2023-08-14

# Path configuration file
PATH_CONFIG_FILE="${HOME}/.sentinel_paths"

# Initialize path config file if it doesn't exist
[[ ! -f "${PATH_CONFIG_FILE}" ]] && touch "${PATH_CONFIG_FILE}"

# Load paths from configuration
load_custom_paths() {
    if [[ -f "${PATH_CONFIG_FILE}" ]]; then
        local path_entry
        while IFS= read -r path_entry; do
            # Skip comments and empty lines
            [[ -z "${path_entry}" || "${path_entry}" =~ ^# ]] && continue
            
            # Only add if directory exists and isn't already in PATH
            if [[ -d "${path_entry}" && ":${PATH}:" != *":${path_entry}:"* ]]; then
                export PATH="${path_entry}:${PATH}"
                [[ "${SENTINEL_QUIET_MODULES:-1}" != "1" ]] && echo "Added custom path: ${path_entry}"
            fi
        done < "${PATH_CONFIG_FILE}"
    fi
}

# Add path to configuration file and current session
add_path() {
    # Default to current directory if no path provided
    local new_path="${1:-$PWD}"
    
    # Prompt if no argument was given
    if [[ -z "$1" ]]; then
        read -p "Do you want to add the current directory ($PWD) to PATH? [Y/n]: " answer
        case "$answer" in
            [Nn]* )
                # User selected 'no', prompt for directory to add
                read -p "Enter the full path you want to add to PATH: " user_path
                [[ -n "$user_path" ]] && new_path="$user_path"
                ;;
        esac
    fi
    
    # Ensure new_path is not empty
    if [[ -z "$new_path" ]]; then
        echo "No path provided. Aborting."
        return 1
    fi
    
    # Resolve to full path
    local full_path=$(readlink -f "$new_path")
    
    # Validate the path exists
    if [[ ! -d "$full_path" ]]; then
        echo "Error: Directory $full_path does not exist."
        read -p "Create this directory? [y/N]: " create_dir
        if [[ "$create_dir" =~ [Yy] ]]; then
            mkdir -p "$full_path" || return 1
            echo "Directory created: $full_path"
        else
            return 1
        fi
    fi
    
    # Check if full_path is already in PATH
    if echo "$PATH" | tr ':' '\n' | grep -Fxq "$full_path"; then
        echo "Directory $full_path is already in PATH."
    else
        # Add to current session
        export PATH="$full_path:$PATH"
        echo "Added $full_path to PATH for current session."
    fi
    
    # Check if already in config file
    if grep -Fxq "$full_path" "$PATH_CONFIG_FILE" 2>/dev/null; then
        echo "Directory $full_path is already in persistent paths configuration."
    else
        # Add to config file
        echo "$full_path" >> "$PATH_CONFIG_FILE"
        echo "Added $full_path to persistent paths configuration."
        
        # Sort and remove duplicates
        sort -u "$PATH_CONFIG_FILE" -o "$PATH_CONFIG_FILE"
    fi
}

# Remove path from configuration
remove_path() {
    local target_path="$1"
    
    # Display menu if no path specified
    if [[ -z "$target_path" ]]; then
        list_paths
        echo
        read -p "Enter the number of the path to remove: " path_num
        
        # Validate input
        if [[ ! "$path_num" =~ ^[0-9]+$ ]]; then
            echo "Invalid selection."
            return 1
        fi
        
        # Get the specified path
        target_path=$(sed -n "${path_num}p" "$PATH_CONFIG_FILE")
        
        if [[ -z "$target_path" ]]; then
            echo "Invalid selection."
            return 1
        fi
    fi
    
    # Resolve to full path if exists
    [[ -e "$target_path" ]] && target_path=$(readlink -f "$target_path")
    
    # Remove from config file
    if grep -Fq "$target_path" "$PATH_CONFIG_FILE"; then
        sed -i "\#^${target_path}#d" "$PATH_CONFIG_FILE"
        echo "Removed $target_path from persistent paths configuration."
        echo "Changes will take effect in new shell sessions."
        echo "To keep using this path in the current session, it remains in your PATH."
    else
        echo "Path $target_path not found in configuration."
    fi
}

# List configured paths
list_paths() {
    echo "Persistent PATH entries:"
    echo "========================"
    
    if [[ ! -s "$PATH_CONFIG_FILE" ]]; then
        echo "No custom paths configured."
        return
    fi
    
    local line_num=1
    local path_entry
    
    while IFS= read -r path_entry; do
        # Skip comments and empty lines
        [[ -z "$path_entry" || "$path_entry" =~ ^# ]] && continue
        
        # Check if path exists
        if [[ -d "$path_entry" ]]; then
            echo -e "$line_num: \033[32m$path_entry\033[0m"
        else
            echo -e "$line_num: \033[31m$path_entry (not found)\033[0m"
        fi
        
        ((line_num++))
    done < "$PATH_CONFIG_FILE"
}

# Function to refresh current PATH using the configuration
refresh_paths() {
    # Get current standard PATH (before custom entries)
    local std_path=$(echo "$PATH" | sed -E 's/(^|:)\/home\/[^:]+\/[^:]+//g')
    
    # Export fresh PATH starting with standard paths
    export PATH="$std_path"
    
    # Reload custom paths
    load_custom_paths
    
    echo "Paths refreshed from configuration."
}

# Create compatibility function
add2path() {
    echo "NOTE: 'add2path' is deprecated, using 'add_path' instead."
    add_path "$@"
}

# Load custom paths at initialization
load_custom_paths 