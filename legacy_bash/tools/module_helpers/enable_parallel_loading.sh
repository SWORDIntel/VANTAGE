#!/usr/bin/env bash
# SENTINEL Parallel Loading Enabler
# This script enables the parallel loading system for SENTINEL modules

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
SENTINEL_ROOT="${SENTINEL_ROOT:-$(cd -- "$SCRIPT_DIR/../.." && pwd -P)}"
MODULES_DIR="${SENTINEL_ROOT}/bash_modules.d"
ENABLED_MODULES="$HOME/.enabled_modules"

if [[ "${SENTINEL_QUIET_MODE:-0}" != "1" && "${SENTINEL_SUPPRESS_MODULE_MESSAGES:-0}" != "1" ]]; then
    echo "=== SENTINEL Parallel Loading Enabler ==="
    echo
fi

# Check if running in bash
if [[ -z "${BASH_VERSION:-}" ]]; then
    echo "ERROR: This script must be run with bash"
    exit 1
fi

# Backup current configuration
if [[ -f "$HOME/.bashrc" ]]; then
    cp "$HOME/.bashrc" "$HOME/.bashrc.backup_$(date +%Y%m%d_%H%M%S)"
    echo "✓ Backed up .bashrc"
fi

# Ensure modules directory exists
if [[ ! -d "$MODULES_DIR" ]]; then
    echo "ERROR: Modules directory not found: $MODULES_DIR"
    exit 1
fi

# Enable required modules
echo "Enabling required modules..."

# Core modules needed for parallel loading
REQUIRED_MODULES=(
    "logging"
    "config_cache"
    "parallel_loader"
    "performance_monitor"
)

# Create or update enabled modules list
touch "$ENABLED_MODULES"

for module in "${REQUIRED_MODULES[@]}"; do
    if ! grep -q "^$module$" "$ENABLED_MODULES" 2>/dev/null; then
        echo "$module" >> "$ENABLED_MODULES"
        echo "  ✓ Enabled: $module"
    else
        echo "  • Already enabled: $module"
    fi
done

# Add parallel loading configuration to bashrc
echo
echo "Configuring parallel loading in .bashrc..."

# Check if parallel loading is already configured
if ! grep -q "SENTINEL_PARALLEL_LOADING" "$HOME/.bashrc" 2>/dev/null; then
    cat >> "$HOME/.bashrc" << 'EOF'

# SENTINEL Parallel Module Loading Configuration
export SENTINEL_PARALLEL_LOADING=1
export SENTINEL_PARALLEL_MAX_JOBS=4
export SENTINEL_MODULE_CACHE_DIR="$HOME/.cache/sentinel/modules"

# Use parallel loading if available
if [[ -f "$HOME/.enabled_modules" ]] && grep -q "parallel_loader" "$HOME/.enabled_modules" 2>/dev/null; then
    # Source the parallel loader first
    if [[ -f "${SENTINEL_ROOT}/bash_modules.d/parallel_loader.module" ]]; then
        source "${SENTINEL_ROOT}/bash_modules.d/parallel_loader.module"
        # Use parallel loading for remaining modules
        parallel_load_modules "$HOME/.enabled_modules"
    fi
fi
EOF
    echo "✓ Added parallel loading configuration"
else
    echo "• Parallel loading already configured"
fi

# Create cache directory
CACHE_DIR="$HOME/.cache/sentinel/modules"
mkdir -p "$CACHE_DIR"
echo "✓ Created cache directory: $CACHE_DIR"

# Test the configuration
echo
echo "Testing parallel loading system..."

# Source the parallel loader module
if source "$MODULES_DIR/parallel_loader.module" 2>/dev/null; then
    echo "✓ Parallel loader module loaded successfully"
    
    # Build initial dependency graph
    echo "Building module dependency graph..."
    if _build_dependency_graph "$MODULES_DIR" 2>/dev/null; then
        echo "✓ Dependency graph built successfully"
    else
        echo "⚠ Warning: Could not build dependency graph"
    fi
else
    echo "ERROR: Failed to load parallel loader module"
    exit 1
fi

# Display configuration summary
echo
echo "=== Configuration Summary ==="
echo "Parallel loading: ENABLED"
echo "Max parallel jobs: ${SENTINEL_PARALLEL_MAX_JOBS:-4}"
echo "Cache directory: $CACHE_DIR"
echo "Enabled modules: $(wc -l < "$ENABLED_MODULES")"
echo

# Provide usage instructions
cat << 'USAGE'
=== Usage Instructions ===

1. To use parallel loading immediately:
   $ source ~/.bashrc

2. To view module dependencies:
   $ show_module_dependencies

3. To monitor performance:
   $ perf-monitor

4. To analyze performance trends:
   $ perf-analyze

5. To clear module cache:
   $ clear_module_cache

6. To disable parallel loading:
   $ export SENTINEL_PARALLEL_LOADING=0

For more information, see:
  "${SENTINEL_ROOT}/docs/internal/improvement_project/team1_performance/parallel_loading.md"
  "${SENTINEL_ROOT}/docs/internal/improvement_project/team1_performance/caching.md"

USAGE

echo
echo "✓ Parallel loading system enabled successfully!"
echo "  Please run 'source ~/.bashrc' or start a new terminal to use it."
