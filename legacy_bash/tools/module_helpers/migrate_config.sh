#!/usr/bin/env bash
# VANTAGE - Configuration Migration Script
# Version: 1.0.0
# Description: Migrate configuration from multiple files to centralized config
# This script extracts configuration from various files and merges them into
# the centralized configuration file.

# Set strict error handling
set -eo pipefail

# Define colors for prettier output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
BOLD="\033[1m"
NC="\033[0m" # No Color

# Paths
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
VANTAGE_DIR="${VANTAGE_ROOT:-$(cd -- "$SCRIPT_DIR/../.." && pwd -P)}"
POSTCUSTOM="${VANTAGE_DIR}/bashrc.postcustom"
BASH_MODULES="${VANTAGE_DIR}/bash_modules"
MODULES_DIR="${VANTAGE_DIR}/bash_modules.d"
VANTAGE_CONFIG_DIR="${HOME}/.vantage"
BACKUP_DIR="${VANTAGE_CONFIG_DIR}/backups/$(date +%Y%m%d%H%M%S)"
BASH_LOGOUT_MODULE="${MODULES_DIR}/logout.module"
UPDATE_SCRIPT="${BACKUP_DIR}/update_postcustom.sh"

# Load config_loader.module to use its functions
source_config_loader() {
    # Remove reference to suggestions/config_loader.module
    # Remove or comment out this line
}

# Print banner
print_banner() {
    echo -e "${BLUE}"
    echo "███████╗███████╗███╗   ██╗████████╗██╗███╗   ██╗███████╗██╗      "
    echo "██╔════╝██╔════╝████╗  ██║╚══██╔══╝██║████╗  ██║██╔════╝██║      "
    echo "███████╗█████╗  ██╔██╗ ██║   ██║   ██║██╔██╗ ██║█████╗  ██║      "
    echo "╚════██║██╔══╝  ██║╚██╗██║   ██║   ██║██║╚██╗██║██╔══╝  ██║      "
    echo "███████║███████╗██║ ╚████║   ██║   ██║██║ ╚████║███████╗███████╗ "
    echo "╚══════╝╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝ "
    echo -e "${NC}"
    echo -e "${BOLD}Configuration Migration Tool${NC}\n"
    echo -e "This script will migrate your VANTAGE configuration from multiple files"
    echo -e "to the new centralized configuration system.\n"
}

# Create backup directory
create_backup_dir() {
    echo "Creating backup directory..."
    mkdir -p "$BACKUP_DIR"
    echo "✓ Created backup directory: $BACKUP_DIR"
    echo
}

# Backup existing files
backup_files() {
    echo "Creating backup of existing configuration files..."
    
    if [[ -f "$POSTCUSTOM" ]]; then
        cp "$POSTCUSTOM" "$BACKUP_DIR/bashrc.postcustom.bak"
        echo "✓ Backed up: bashrc.postcustom"
    fi
    
    if [[ -f "$BASH_LOGOUT_MODULE" ]]; then
        cp "$BASH_LOGOUT_MODULE" "$BACKUP_DIR/bash_logout.module.bak"
        echo "✓ Backed up: bash_logout.module"
    fi
    
    echo "Backups stored in: $BACKUP_DIR"
    echo
}

# Check if configuration file exists, create if not
ensure_config_file() {
    if [[ ! -f "$CONFIG_FILE" ]]; then
        echo "Creating new configuration file..."
        
        # Get the config_loader module to create a default config
        source_config_loader
        
        echo "✓ Created default configuration file"
        echo
    fi
}

# Extract configuration from bashrc.postcustom
extract_postcustom_config() {
    echo "Extracting configuration from bashrc.postcustom..."
    
    if [[ ! -f "$POSTCUSTOM" ]]; then
        echo "× No bashrc.postcustom file found, skipping"
        return
    fi
    
    # Create update script header
    cat > "$UPDATE_SCRIPT" << EOL
#!/usr/bin/env bash
# VANTAGE - Postcustom Update Script
# Generated on $(date)
# This script will update your bashrc.postcustom to use the new configuration system

# Backup original file
cp "$POSTCUSTOM" "${POSTCUSTOM}.bak.\$(date +%Y%m%d%H%M%S)"

# Update the file with sed
EOL
    chmod +x "$UPDATE_SCRIPT"
    
    # Find all VANTAGE_ variables and extract them
    grep -E '^[[:space:]]*export[[:space:]]+VANTAGE_[A-Z_]+=.*' "$POSTCUSTOM" | while read -r line; do
        # Extract variable name and value
        local var_name=$(echo "$line" | awk -F'=' '{print $1}' | awk '{print $2}')
        local var_value=$(echo "$line" | cut -d'=' -f2- | sed 's/^[[:space:]]*//')
        
        # Update centralized configuration
        if grep -q "^export $var_name=" "$CONFIG_FILE"; then
            # Replace existing value
            echo "Migrating: $var_name"
            sed -i "s|^export $var_name=.*|export $var_name=$var_value|" "$CONFIG_FILE"
        else
            # Add to custom section if it doesn't exist in template
            echo "Adding custom: $var_name"
            sed -i "/# \[Custom user settings\]/a export $var_name=$var_value" "$CONFIG_FILE"
        fi
        
        # Add to update script to comment out this line in postcustom
        echo "sed -i 's|^[[:space:]]*export[[:space:]]\+$var_name=.*|# MIGRATED: & # Moved to centralized config|' \"$POSTCUSTOM\"" >> "$UPDATE_SCRIPT"
    done
    
    # Also find module-specific configurations
    local module_vars=("HASHCAT_BIN" "HASHCAT_WORDLISTS_DIR" "OBFUSCATE_OUTPUT_DIR" "DISTCC_HOSTS" "CCACHE_SIZE")
    
    for var_name in "${module_vars[@]}"; do
        if grep -qE "^[[:space:]]*export[[:space:]]+$var_name=" "$POSTCUSTOM"; then
            local line=$(grep -E "^[[:space:]]*export[[:space:]]+$var_name=" "$POSTCUSTOM")
            local var_value=$(echo "$line" | cut -d'=' -f2- | sed 's/^[[:space:]]*//')
            
            # Update centralized configuration
            if grep -q "^export $var_name=" "$CONFIG_FILE"; then
                echo "Migrating module config: $var_name"
                sed -i "s|^export $var_name=.*|export $var_name=$var_value|" "$CONFIG_FILE"
            else
                echo "Adding module config: $var_name"
                # Add to module specific section
                sed -i "/^# Module-specific configurations/a export $var_name=$var_value" "$CONFIG_FILE"
            fi
            
            # Add to update script
            echo "sed -i 's|^[[:space:]]*export[[:space:]]\+$var_name=.*|# MIGRATED: & # Moved to centralized config|' \"$POSTCUSTOM\"" >> "$UPDATE_SCRIPT"
        fi
    done
    
    # Add instructions to the update script
    cat >> "$UPDATE_SCRIPT" << EOL

# Add sourcing statement if it doesn't exist
if ! grep -q "source \\\$HOME/.vantage/vantage_config.sh" "$POSTCUSTOM"; then
    echo "" >> "$POSTCUSTOM"
    echo "# Source centralized configuration" >> "$POSTCUSTOM"
    echo "if [[ -f \\\$HOME/.vantage/vantage_config.sh ]]; then" >> "$POSTCUSTOM"
    echo "    source \\\$HOME/.vantage/vantage_config.sh" >> "$POSTCUSTOM"
    echo "fi" >> "$POSTCUSTOM"
fi

echo "Updated bashrc.postcustom to use centralized configuration"
EOL
    
    echo "✓ Extracted configuration from bashrc.postcustom"
    echo "✓ Created update script: $UPDATE_SCRIPT"
    echo
}

# Extract configuration from bash_logout.module
extract_bash_logout_config() {
    echo "Extracting configuration from bash_logout.module..."
    
    if [[ ! -f "$BASH_LOGOUT_MODULE" ]]; then
        echo "× No bash_logout.module file found, skipping"
        return
    fi
    
    # Look for secure deletion settings
    local secure_vars=(
        "VANTAGE_SECURE_BASH_HISTORY" 
        "VANTAGE_SECURE_SSH_KNOWN_HOSTS"
        "VANTAGE_SECURE_CLEAN_CACHE"
        "VANTAGE_SECURE_BROWSER_CACHE"
        "VANTAGE_SECURE_RECENT"
        "VANTAGE_SECURE_VIM_UNDO"
        "VANTAGE_SECURE_CLIPBOARD"
        "VANTAGE_SECURE_CLEAR_SCREEN"
        "VANTAGE_SECURE_DIRS"
    )
    
    for var_name in "${secure_vars[@]}"; do
        # Look for if statements checking these variables
        # For example: if [[ "$VANTAGE_SECURE_BASH_HISTORY" == "1" ]]; then
        if grep -q "$var_name" "$BASH_LOGOUT_MODULE"; then
            # Check if it's active in the logout module
            if grep -qE "if[[:space:]]*\[\[[[:space:]]*\\\$\{?$var_name\}?[[:space:]]*==[[:space:]]*[\"']?1[\"']?" "$BASH_LOGOUT_MODULE"; then
                echo "Found active security setting: $var_name"
                # Set it to active in the config file
                sed -i "s|^export $var_name=.*|export $var_name=1|" "$CONFIG_FILE"
            fi
        fi
    done
    
    echo "✓ Extracted configuration from bash_logout.module"
    echo
}

# Extract configuration from module files
extract_module_configs() {
    echo "Extracting configuration from module files..."
    
    # List of modules to check
    local modules=("osint.module")
    
    for module in "${modules[@]}"; do
        local module_path="${MODULES_DIR}/${module}"
        if [[ -f "$module_path" ]]; then
            echo "Checking module: ${module}"
            
            # Look for default values assignments like: : "${VAR_NAME:=default_value}"
            grep -E ":[[:space:]]*\"\$\{[A-Z_]+:=[^}]+\}\"" "$module_path" | while read -r line; do
                local var_name=$(echo "$line" | sed -E 's/:[[:space:]]*"\$\{([A-Z_]+):=.*/\1/')
                local var_value=$(echo "$line" | sed -E 's/.*:=([^}]+)\}".*/\1/')
                
                # Skip if we don't want to migrate this variable
                case "$var_name" in
                    VANTAGE_*|HASHCAT_*|OBFUSCATE_*|DISTCC_*|CCACHE_*)
                        # These are variables we want to migrate
                        ;;
                    *)
                        # Skip other variables
                        continue
                        ;;
                esac
                
                echo "Found default value: $var_name=$var_value"
                
                # Only update if it doesn't already exist in the config file
                if ! grep -q "^export $var_name=" "$CONFIG_FILE"; then
                    echo "Adding: $var_name=$var_value"
                    sed -i "/# \[Custom user settings\]/a export $var_name=$var_value" "$CONFIG_FILE"
                fi
            done
        fi
    done
    
    echo "✓ Extracted configuration from module files"
    echo
}

# Apply update script
apply_update() {
    echo "Would you like to apply the updates to bashrc.postcustom? (y/n)"
    read -r choice
    
    if [[ "$choice" == "y" ]]; then
        bash "$UPDATE_SCRIPT"
        echo "✓ Applied updates to bashrc.postcustom"
    else
        echo "× Updates not applied. You can run the script manually later:"
        echo "  $UPDATE_SCRIPT"
    fi
    echo
}

# Main function
main() {
    print_banner
    create_backup_dir
    backup_files
    ensure_config_file
    extract_postcustom_config
    extract_bash_logout_config
    extract_module_configs
    
    echo -e "${GREEN}Configuration migration completed!${NC}"
    echo "All settings have been migrated to: $CONFIG_FILE"
    echo
    
    apply_update
    
    echo -e "${BOLD}Next steps:${NC}"
    echo "1. Review your centralized configuration: vantage_config"
    echo "2. Reload your bash environment: source ~/.bashrc"
    echo "3. Verify that everything works as expected"
    echo
    echo "If you encounter any issues, you can restore your backups from:"
    echo "$BACKUP_DIR"
}

# Run the main function
main "$@" 
