#!/usr/bin/env bash

# Module dependency checker for VANTAGE
# Analyzes module dependencies and checks for missing declarations

echo "=== VANTAGE Module Dependency Analysis ==="
echo

export VANTAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

MODULES_DIR="${VANTAGE_ROOT}/bash_modules.d"
ISSUES_FOUND=0

# Colors for output
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
NC="\033[0m"

# Map of known function providers
declare -A FUNCTION_PROVIDERS=(
    ["vantage_log_info"]="logging"
    ["vantage_log_warning"]="logging"
    ["vantage_log_error"]="logging"
    ["_log_info"]="logging"
    ["_log_warning"]="logging"
    ["_log_error"]="logging"
    ["CONFIG"]="config_cache"
    ["source_cached"]="config_cache"
    ["_config_cache_enabled"]="config_cache"
)

# Check each module
for module_file in "$MODULES_DIR"/*.module "$MODULES_DIR"/*.sh; do
    [[ ! -f "$module_file" ]] && continue
    
    module_name=$(basename "$module_file" | sed 's/\.\(module\|sh\)$//')
    
    # Skip non-module files
    [[ "$module_name" == "migrate_config" || "$module_name" == "install-autocomplete" ]] && continue
    
    echo -e "${BLUE}Checking module: $module_name${NC}"
    
    # Get declared dependencies
    declared_deps=""
    if grep -q "VANTAGE_MODULE_DEPENDENCIES=" "$module_file"; then
        declared_deps=$(grep "VANTAGE_MODULE_DEPENDENCIES=" "$module_file" | head -n1 | sed 's/.*="\(.*\)".*/\1/')
    fi
    
    # Check for usage of functions from other modules
    missing_deps=""
    
    # Check for logging functions
    if grep -qE "(vantage_log_info|vantage_log_warning|vantage_log_error|_log_info|_log_warning|_log_error)" "$module_file"; then
        # Check if module defines its own logging functions
        if ! grep -q "^\(function \)\?vantage_log_info\(\(\)\)\?\s*{" "$module_file" && \
           ! grep -q "^\(function \)\?_vantage_log_info\(\(\)\)\?\s*{" "$module_file"; then
            if [[ ! "$declared_deps" =~ "logging" ]]; then
                missing_deps+="logging "
            fi
        fi
    fi
    
    # Check for config_cache functions
    if grep -qE "(source_cached|CONFIG\[|_config_cache_enabled)" "$module_file"; then
        if [[ ! "$declared_deps" =~ "config_cache" ]]; then
            missing_deps+="config_cache "
        fi
    fi
    
    # Report findings
    if [[ -n "$declared_deps" ]]; then
        echo "  Declared dependencies: $declared_deps"
    else
        echo "  No dependencies declared"
    fi
    
    if [[ -n "$missing_deps" ]]; then
        echo -e "  ${RED}Missing dependencies: $missing_deps${NC}"
        ((ISSUES_FOUND++))
    else
        echo -e "  ${GREEN}Dependencies OK${NC}"
    fi
    
    echo
done

# Check module load order
echo -e "${BLUE}Checking module load order...${NC}"

if [[ -f "${VANTAGE_ROOT}/.bash_modules" ]]; then
    echo "Module load order from .bash_modules:"
    
    # Check that dependencies are loaded before modules that need them
    loaded_modules=()
    while IFS= read -r module; do
        # Skip comments and empty lines
        [[ -z "$module" || "$module" =~ ^# ]] && continue
        
        module_file="$MODULES_DIR/$module.module"
        [[ ! -f "$module_file" ]] && module_file="$MODULES_DIR/$module.sh"
        
        if [[ -f "$module_file" ]]; then
            # Get dependencies
            deps=""
            if grep -q "VANTAGE_MODULE_DEPENDENCIES=" "$module_file"; then
                deps=$(grep "VANTAGE_MODULE_DEPENDENCIES=" "$module_file" | head -n1 | sed 's/.*="\(.*\)".*/\1/')
            fi
            
            # Check if dependencies are loaded
            for dep in $deps; do
                if [[ ! " ${loaded_modules[@]} " =~ " ${dep} " ]]; then
                    echo -e "  ${YELLOW}Warning: $module requires $dep, but it's loaded after${NC}"
                fi
            done
        fi
        
        loaded_modules+=("$module")
    done < "${VANTAGE_ROOT}/.bash_modules"
else
    echo -e "${YELLOW}No .bash_modules file found${NC}"
fi

echo
echo "=== Summary ==="
if [[ $ISSUES_FOUND -eq 0 ]]; then
    echo -e "${GREEN}All module dependencies appear to be properly declared!${NC}"
else
    echo -e "${RED}Found $ISSUES_FOUND modules with missing dependency declarations${NC}"
fi