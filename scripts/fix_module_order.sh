#!/usr/bin/env bash

# Script to fix module loading order based on dependencies

echo "=== Fixing VANTAGE Module Load Order ==="
echo

export VANTAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

MODULES_DIR="${VANTAGE_ROOT}/bash_modules.d"
MODULES_FILE="${VANTAGE_ROOT}/.bash_modules"

# Create a dependency graph
declare -A MODULE_DEPS
declare -A MODULE_EXISTS

# Read all modules and their dependencies
for module_file in "$MODULES_DIR"/*.module "$MODULES_DIR"/*.sh; do
    [[ ! -f "$module_file" ]] && continue
    
    module_name=$(basename "$module_file" | sed 's/\.\(module\|sh\)$//')
    
    # Skip non-module files
    [[ "$module_name" == "migrate_config" || "$module_name" == "install-autocomplete" ]] && continue
    
    MODULE_EXISTS["$module_name"]=1
    
    # Get dependencies
    if grep -q "VANTAGE_MODULE_DEPENDENCIES=" "$module_file"; then
        deps=$(grep "VANTAGE_MODULE_DEPENDENCIES=" "$module_file" | head -n1 | sed 's/.*="\(.*\)".*/\1/')
        MODULE_DEPS["$module_name"]="$deps"
    else
        MODULE_DEPS["$module_name"]=""
    fi
done

# Function to perform topological sort
topological_sort() {
    local -A in_degree
    local -A adj_list
    local -a queue
    local -a result
    
    # Initialize in-degree for all modules
    for module in "${!MODULE_EXISTS[@]}"; do
        in_degree["$module"]=0
        adj_list["$module"]=""
    done
    
    # Build adjacency list and calculate in-degrees
    for module in "${!MODULE_DEPS[@]}"; do
        for dep in ${MODULE_DEPS["$module"]}; do
            if [[ -n "${MODULE_EXISTS[$dep]}" ]]; then
                adj_list["$dep"]+="$module "
                ((in_degree["$module"]++))
            fi
        done
    done
    
    # Find all modules with no dependencies
    for module in "${!in_degree[@]}"; do
        if [[ ${in_degree["$module"]} -eq 0 ]]; then
            queue+=("$module")
        fi
    done
    
    # Process the queue
    while [[ ${#queue[@]} -gt 0 ]]; do
        # Dequeue
        local current="${queue[0]}"
        queue=("${queue[@]:1}")
        result+=("$current")
        
        # Process all modules that depend on current
        for dependent in ${adj_list["$current"]}; do
            ((in_degree["$dependent"]--))
            if [[ ${in_degree["$dependent"]} -eq 0 ]]; then
                queue+=("$dependent")
            fi
        done
    done
    
    # Output the sorted order
    printf '%s\n' "${result[@]}"
}

# Get the correct order
echo "Calculating optimal module load order..."
SORTED_MODULES=$(topological_sort)

# Read current module list from .bash_modules
declare -a CURRENT_MODULES
declare -A MODULE_IN_LIST

if [[ -f "$MODULES_FILE" ]]; then
    while IFS= read -r line; do
        # Skip comments and empty lines
        [[ -z "$line" || "$line" =~ ^# ]] && continue
        MODULE_IN_LIST["$line"]=1
    done < "$MODULES_FILE"
fi

# Create new module list with correct order
NEW_MODULES_FILE="${MODULES_FILE}.new"

cat > "$NEW_MODULES_FILE" << 'EOF'
# Always first (enforced by loader)
blesh_installer

EOF

# Add modules in dependency order
echo "# Core modules in dependency order" >> "$NEW_MODULES_FILE"

# First add modules without dependencies
for module in $SORTED_MODULES; do
    if [[ "${MODULE_IN_LIST[$module]}" == "1" && -z "${MODULE_DEPS[$module]}" ]]; then
        echo "$module" >> "$NEW_MODULES_FILE"
        unset MODULE_IN_LIST["$module"]
    fi
done

echo "" >> "$NEW_MODULES_FILE"
echo "# Modules with dependencies" >> "$NEW_MODULES_FILE"

# Then add modules with dependencies in sorted order
for module in $SORTED_MODULES; do
    if [[ "${MODULE_IN_LIST[$module]}" == "1" ]]; then
        echo "$module" >> "$NEW_MODULES_FILE"
    fi
done

echo "" >> "$NEW_MODULES_FILE"
echo "# Add other modules below this line" >> "$NEW_MODULES_FILE"

# Show the changes
echo
echo "Current vs New module order:"
echo "==========================="
if command -v diff &>/dev/null; then
    diff -u "$MODULES_FILE" "$NEW_MODULES_FILE" || true
fi

echo
read -p "Apply the new module order? (y/n): " apply
if [[ "$apply" == "y" ]]; then
    # Backup current file
    cp "$MODULES_FILE" "${MODULES_FILE}.bak"
    # Apply new order
    mv "$NEW_MODULES_FILE" "$MODULES_FILE"
    echo "Module order updated! Backup saved to ${MODULES_FILE}.bak"
else
    rm "$NEW_MODULES_FILE"
    echo "Changes discarded."
fi