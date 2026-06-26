# VANTAGE Troubleshooting Guide

## Common Issues and Solutions

### 1. Bash Startup Issues

#### Problem: Bash takes too long to start
```bash
# Symptoms:
# - Noticeable delay when opening new terminal
# - Shell feels sluggish

# Diagnosis:
time bash -i -c exit  # Measure startup time
MODULE_TIMING=1 bash -i -c exit  # See module load times

# Solutions:
# 1. Disable unused modules
edit ~/.bashrc  # Set VANTAGE_MODULES_DISABLED="heavy_module,another_module"

# 2. Enable lazy loading
export MODULE_LAZY_LOAD=true

# 3. Clear cache if corrupted
rm -rf ~/.cache/vantage/
```

#### Problem: Bash won't start at all
```bash
# Emergency recovery:
bash --noprofile --norc  # Start without config

# Use emergency bashrc:
cp /opt/github/VANTAGE/scripts/emergency.bashrc ~/.bashrc

# Debug the issue:
bash -x  # Trace execution to find error
```

### 2. Module Loading Issues

#### Problem: Module fails to load
```bash
# Error: "Module X failed to load"

# Check module syntax:
bash -n bash_modules.d/problem_module.module

# Check dependencies:
grep "MODULE_DEPS" bash_modules.d/problem_module.module

# Test in isolation:
bash --norc -c 'source bash_modules.d/problem_module.module'

# Enable debug logging:
MODULE_DEBUG=1 bash_modules load problem_module
```

#### Problem: Module conflicts
```bash
# Symptoms: Functions overridden, aliases conflict

# Find conflicting definitions:
type -a conflicting_function
alias | grep conflicting_alias

# Check load order:
echo "${LOADED_MODULES[@]}"

# Solution: Adjust module load order in bashrc.postcustom
VANTAGE_MODULE_LOAD_ORDER="module1,module2,module3"
```

### 3. Python Integration Issues

#### Problem: Python tools not working
```bash
# Check Python version:
python3 --version  # Need 3.8+

# Check dependencies:
pip3 list | grep -E "transformers|torch|numpy"

# Install missing dependencies:
pip3 install -r /opt/github/VANTAGE/requirements.txt

# Test Python integration:
python3 /opt/github/VANTAGE/contrib/vantage_chat.py --test
```

#### Problem: ML features disabled
```bash
# Enable ML features:
export VANTAGE_ML_ENABLED=true

# Download required models:
python3 /opt/github/VANTAGE/gitstar/download_models.py

# Check model directory:
ls -la ~/.cache/vantage/models/
```

### 4. Performance Issues

#### Problem: Commands are slow
```bash
# Profile command execution:
time your_slow_command

# Check if caching is enabled:
echo $CONFIG_CACHE_ENABLED

# Clear stale cache:
find ~/.cache/vantage -mtime +7 -delete

# Disable heavy features temporarily:
VANTAGE_FEATURES_MINIMAL=true bash
```

### 5. Security Module Issues

#### Problem: HMAC verification fails
```bash
# Error: "Module failed HMAC verification"

# Regenerate HMAC signatures:
vantage hmac --regenerate

# Disable HMAC temporarily (not recommended):
export ENABLE_HMAC_VERIFICATION=false

# Check file permissions:
ls -la bash_modules.d/*.module | grep -v "rw-r--r--"
```

### 6. Installation Issues

#### Problem: Install script fails
```bash
# Run with debug output:
bash -x install.sh

# Check prerequisites:
./install.sh --check-only

# Manual installation:
# 1. Copy files manually
cp -r bash_* ~/.config/vantage/
# 2. Add to .bashrc manually
echo 'source ~/.config/vantage/bashrc' >> ~/.bashrc
```

## Diagnostic Commands

### System Information
```bash
# VANTAGE diagnostic info:
vantage --diagnose

# Manual diagnostics:
cat > /tmp/vantage_diag.sh << 'EOF'
#!/bin/bash
echo "=== VANTAGE Diagnostics ==="
echo "Date: $(date)"
echo "User: $USER"
echo "Shell: $SHELL"
echo "Bash version: $BASH_VERSION"
echo "OS: $(uname -a)"
echo ""
echo "=== VANTAGE Info ==="
echo "Installation dir: $(dirname $(readlink -f ${BASH_SOURCE[0]}))"
echo "Loaded modules: ${LOADED_MODULES[@]}"
echo "Python3: $(command -v python3)"
echo "Cache dir: ~/.cache/vantage/"
echo ""
echo "=== Environment ==="
env | grep VANTAGE_ | sort
EOF

bash /tmp/vantage_diag.sh
```

### Module Diagnostics
```bash
# List all modules and status:
for module in bash_modules.d/*.module; do
    name=$(basename "$module" .module)
    if is_module_loaded "$name"; then
        echo "✓ $name (loaded)"
    else
        echo "✗ $name (not loaded)"
    fi
done
```

### Performance Diagnostics
```bash
# Measure module load times:
for module in "${LOADED_MODULES[@]}"; do
    (time source "bash_modules.d/${module}.module") 2>&1 | \
    grep real | \
    awk -v m="$module" '{print m ": " $2}'
done | sort -k2 -n
```

## Recovery Procedures

### 1. Safe Mode
```bash
# Start VANTAGE in safe mode:
VANTAGE_SAFE_MODE=true bash

# Safe mode disables:
# - Non-essential modules
# - Python integration
# - Heavy features
```

### 2. Reset to Defaults
```bash
# Backup current config:
cp ~/.bashrc ~/.bashrc.backup.$(date +%s)

# Reset to defaults:
/opt/github/VANTAGE/install.sh --reset

# Or manually:
cp /opt/github/VANTAGE/bashrc ~/.bashrc
rm -rf ~/.cache/vantage/
```

### 3. Module Recovery
```bash
# Disable all modules:
export VANTAGE_MODULES_DISABLED="*"

# Enable modules one by one:
unset VANTAGE_MODULES_DISABLED
for module in bash_modules.d/*.module; do
    name=$(basename "$module" .module)
    echo "Testing $name..."
    if bash -c "source $module"; then
        echo "✓ $name is OK"
    else
        echo "✗ $name has issues"
    fi
done
```

## Log Files and Debugging

### 1. Enable Logging
```bash
# In ~/.bashrc:
export VANTAGE_LOG_LEVEL=DEBUG
export VANTAGE_LOG_FILE=~/.cache/vantage/debug.log
```

### 2. View Logs
```bash
# Tail logs:
tail -f ~/.cache/vantage/debug.log

# Search for errors:
grep -i error ~/.cache/vantage/*.log

# Module-specific logs:
grep "module_name" ~/.cache/vantage/debug.log
```

### 3. Trace Execution
```bash
# Trace specific module:
bash -x bash_modules.d/problem_module.module 2>/tmp/trace.log

# Trace entire startup:
PS4='+ ${BASH_SOURCE##*/}:${LINENO}: ' bash -x 2>/tmp/startup_trace.log
```

## Getting Help

### 1. Built-in Help
```bash
# VANTAGE help:
vantage --help

# Module help:
module_name --help

# Function help:
type function_name
```

### 2. Check Documentation
```bash
# Browse documentation:
ls -la /opt/github/VANTAGE/*.md
ls -la /opt/github/VANTAGE/.claude/

# Search documentation:
grep -r "search term" /opt/github/VANTAGE/.claude/
```

### 3. Community Support
When reporting issues, include:
1. Output of `vantage --diagnose`
2. Error messages (exact)
3. Steps to reproduce
4. What you expected vs what happened
5. Any recent changes made

## Prevention Tips

1. **Always test changes**: Run `scripts/vantage_postinstall_check.sh` after modifications
2. **Keep backups**: Backup working configs before major changes
3. **Use version control**: Track your customizations in git
4. **Document changes**: Note what you modify and why
5. **Update carefully**: Test updates in a separate environment first

Remember: Most issues can be resolved by understanding the module loading order and checking dependencies!