# SENTINEL Module Lazy Loading Implementation

## Overview

This document describes the lazy loading implementation for SENTINEL modules, designed to significantly improve shell startup performance by deferring the loading of heavy modules until they are actually needed.

## Architecture

### Core Components

1. **Module Classification System**
   - Core modules: Essential modules loaded eagerly at startup
   - Lazy modules: Heavy modules loaded on-demand when first used

2. **Lazy Loading Registry**
   - `SENTINEL_LOADED_MODULES`: Tracks all loaded modules
   - `SENTINEL_LAZY_MODULES`: Tracks lazy-loaded modules and their state
   - `SENTINEL_MODULE_LOADING_STRATEGY`: Configuration for loading behavior

3. **Proxy Function System**
   - Creates lightweight proxy functions for lazy modules
   - Proxies intercept first use and trigger actual module loading
   - After loading, proxies are replaced with real functions

### Configuration

The lazy loading system is configured through `/opt/github/SENTINEL/bash_modules.d/lazy_loading.conf`:

```bash
# Global toggle
export SENTINEL_LAZY_LOADING_ENABLED=1

# Core modules (always loaded eagerly)
SENTINEL_CORE_MODULES=(
    "blesh_installer"
    "autocomplete"
    "logging"
    "shell_security"
    "config_cache"
    "module_manager"
    "bash_logout"
    "command_chains"
    "fuzzy_correction"
    "fzf"
)

# Heavy modules (lazy loaded)
SENTINEL_LAZY_LOAD_MODULES=(
    "sentinel_ml"
    "sentinel_ml_enhanced"
    "sentinel_osint"
    "sentinel_cybersec_ml"
    "sentinel_chat"
    "sentinel_gitstar"
    "sentinel_context"
    "sentinel_markov"
    "sentinel_smallllm"
    "hashcat"
    "obfuscate"
)
```

### Implementation Details

#### 1. Module Detection (`is_lazy_load_module`)

```bash
is_lazy_load_module() {
    local module_name="$1"
    
    # Check global toggle
    if [[ "${SENTINEL_LAZY_LOADING_ENABLED:-1}" == "0" ]]; then
        return 1
    fi
    
    # Check module-specific override
    local override_var="SENTINEL_${module_name^^}_LAZY_LOAD"
    if [[ -n "${!override_var}" ]]; then
        [[ "${!override_var}" == "1" ]] && return 0 || return 1
    fi
    
    # Check against lazy/core lists
    # ...
}
```

#### 2. Proxy Creation (`create_lazy_proxy`)

Creates lightweight proxy functions that:
- Intercept first use of module commands
- Load the actual module on demand
- Replace themselves with real functions
- Show loading messages (configurable)

Example proxy for ML module:
```bash
ml_suggest() {
    if [[ "${SENTINEL_LAZY_MODULES[sentinel_ml]}" != "loaded" ]]; then
        emsg "[LAZY] Loading ML module for suggestions..."
        module_enable "sentinel_ml" "0" "lazy"
        SENTINEL_LAZY_MODULES["sentinel_ml"]="loaded"
    fi
    if type ml_suggest &>/dev/null; then
        ml_suggest "$@"
    fi
}
```

#### 3. Module Loading Integration

The `module_enable` function has been enhanced to:
- Check if a module should be lazy loaded during startup
- Create proxy functions instead of loading the module
- Mark modules as "lazy" in the loaded modules registry
- Handle lazy loading when called by proxy functions

### Usage

#### Basic Commands

1. **Check lazy loading status:**
   ```bash
   module_lazy_status
   ```

2. **List all modules with lazy loading indicators:**
   ```bash
   module_list
   ```
   
   Status indicators:
   - `[E]` - Enabled at startup
   - `[L]` - Loaded now
   - `[*]` - Both enabled and loaded
   - `[z]` - Lazy (ready but not loaded)
   - `[Z]` - Lazy (loaded on demand)

3. **Force load a lazy module:**
   ```bash
   module_enable sentinel_ml 1
   ```

#### Configuration Options

1. **Disable lazy loading globally:**
   ```bash
   export SENTINEL_LAZY_LOADING_ENABLED=0
   ```

2. **Force specific module to load eagerly:**
   ```bash
   export SENTINEL_SENTINEL_ML_LAZY_LOAD=0
   ```

3. **Force specific module to load lazily:**
   ```bash
   export SENTINEL_FZF_LAZY_LOAD=1
   ```

4. **Control loading messages:**
   ```bash
   export SENTINEL_LAZY_SHOW_LOADING=0  # Hide loading messages
   ```

### Performance Benefits

1. **Startup Time Reduction**
   - Core modules only: ~200-300ms
   - All modules (before): ~1-2 seconds
   - Estimated improvement: 70-85% faster startup

2. **Memory Usage**
   - Only loaded modules consume memory
   - Python dependencies not loaded until needed
   - Model files not loaded until ML features used

3. **First-Use Delay**
   - Small delay on first use of lazy module (~100-500ms)
   - Subsequent uses have no delay
   - Trade-off: Fast startup vs. slight first-use delay

### Module Categories

#### Always Eager (Core)
- `blesh_installer` - Line editor enhancements
- `autocomplete` - Tab completion
- `logging` - Audit trail
- `shell_security` - Security features
- `config_cache` - Performance optimization
- `module_manager` - Module system itself

#### Default Lazy (Heavy)
- **ML/AI Modules**: Require Python, models, libraries
  - `sentinel_ml`, `sentinel_ml_enhanced`
  - `sentinel_context`, `sentinel_markov`
  - `sentinel_smallllm`
  
- **Security/Analysis**: External tools, heavy operations
  - `sentinel_osint` - OSINT tools
  - `sentinel_cybersec_ml` - Security ML
  - `hashcat` - Password cracking
  - `obfuscate` - Code obfuscation
  
- **Interactive**: Not needed until explicitly used
  - `sentinel_chat` - LLM chat
  - `sentinel_gitstar` - GitHub analysis

### Troubleshooting

1. **Module not loading when expected:**
   - Check `module_lazy_status` to see if it's marked as lazy
   - Verify proxy functions exist: `type ml_suggest`
   - Check for errors in module file

2. **Want to preload all modules:**
   ```bash
   for module in "${!SENTINEL_LAZY_MODULES[@]}"; do
       [[ "${SENTINEL_LAZY_MODULES[$module]}" != "loaded" ]] && \
           module_enable "$module" 0 "preload"
   done
   ```

3. **Debug lazy loading:**
   ```bash
   export SENTINEL_DEBUG_MODULES=1
   export SENTINEL_LAZY_SHOW_LOADING=1
   ```

### Future Enhancements

1. **Idle-time preloading**: Load modules during shell idle time
2. **Predictive loading**: Load modules based on usage patterns
3. **Module groups**: Load related modules together
4. **Persistent state**: Remember loaded modules across sessions
5. **Dependency-aware lazy loading**: Load dependencies just-in-time

### Migration Guide

For existing users:
1. Lazy loading is enabled by default
2. No changes needed to module files
3. Existing configurations work unchanged
4. To disable: `export SENTINEL_LAZY_LOADING_ENABLED=0`

For module developers:
1. Ensure modules are self-contained
2. Avoid side effects during sourcing
3. Export main functions explicitly
4. Test with lazy loading enabled