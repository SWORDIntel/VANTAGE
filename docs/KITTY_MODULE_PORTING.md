# Kitty Pathway Module Porting Guide

This document describes how all VANTAGE modules and addons have been ported to work with the kitty primary CLI pathway.

## Overview

All modules have been updated to:
1. Detect kitty terminal environment
2. Optimize for kitty's GPU acceleration
3. Use kitty-specific features when available
4. Fall back gracefully when kitty is not available

## Module Updates

### Core Modules

#### `kitty_integration.module` (NEW)
- Provides `vantage_is_kitty()` function for terminal detection
- Provides `vantage_kitty_set_title()` for window management
- Enables GPU acceleration flags automatically
- Sets up 256-color support

#### `autocomplete.module`
- Updated to work without BLE.sh dependency
- Detects kitty and optimizes accordingly
- Snippet engine works with bash functions in kitty mode

#### `fzf.module`
- Enhanced with kitty-specific color schemes
- Uses kitty's image preview capabilities (`kitty +kitten icat`)
- Optimized FZF options for kitty's rendering

#### `snippets.module`
- Ported from BLE.sh dependency to bash functions
- Works seamlessly in kitty using function-based expansion
- Maintains HMAC verification for security

### Vantage Modules

#### `vantage_git_tui.module`
- Color functions optimized for kitty
- Enhanced display capabilities

#### `vantage_chat.module`
- Compatible with kitty (no terminal-specific code)

#### `vantage_ml.module`
- Compatible with kitty (no terminal-specific code)

### Plugins

#### `docker_integration.plugin`
- Monitor functions optimized for kitty's clear performance
- Uses kitty-aware display updates

#### `k8s_integration.plugin`
- Monitor functions optimized for kitty
- Resource display enhanced for kitty rendering

#### `git_integration.plugin`
- Fully compatible with kitty
- No terminal-specific dependencies

### Integration Modules

#### `fabric_integration.module`
- Uses kitty's image display for documentation
- Falls back gracefully to standard display

## Kitty-Specific Features

### GPU Acceleration
All modules automatically enable GPU acceleration when kitty is detected:
```bash
export VANTAGE_KITTY_GPU_ACCEL=1
export VANTAGE_TERMINAL_COLORS=256
```

### Image Display
Modules can use kitty's image display capabilities:
```bash
kitty +kitten icat --clear --transfer-mode=memory <image>
```

### Window Management
Modules can set window and tab titles:
```bash
vantage_kitty_set_title "My Project"
vantage_kitty_set_tab_title "Git Operations"
```

## Module Loading Order

The kitty pathway loads modules in this order:

1. **Early**: `kitty_integration.module` - Provides detection functions
2. **Core**: `module_manager.module`, `parallel_loader.module`
3. **All enabled modules**: Loaded via parallel loader
4. **Plugins**: Loaded from `bash_modules.d/plugins/`

## Detection Pattern

Modules should use this pattern to detect kitty:

```bash
if type vantage_is_kitty &>/dev/null && vantage_is_kitty; then
    # Kitty-specific optimizations
    export OPTION="kitty_value"
else
    # Fallback for other terminals
    export OPTION="default_value"
fi
```

## Fallback Behavior

All modules maintain backward compatibility:
- If kitty is not detected, modules work normally
- BLE.sh dependencies have been removed or made optional
- Terminal-specific features degrade gracefully

## Testing

To test kitty integration:

1. Ensure kitty is installed and running
2. Check detection: `vantage_is_kitty && echo "Kitty detected"`
3. Verify GPU acceleration: `echo $VANTAGE_KITTY_GPU_ACCEL`
4. Test module loading: Check that all modules load without errors

## Migration Notes

### From BLE.sh to Kitty
- Snippets now use bash functions instead of `ble-sabbrev`
- Autocomplete works without BLE.sh
- All features available in kitty pathway

### From Bash Pathway
- All modules work identically
- Additional kitty optimizations enabled automatically
- No configuration changes needed

## Future Enhancements

Potential kitty-specific enhancements:
- Remote control via kitty's remote control protocol
- Image previews in FZF and file browsers
- Enhanced color schemes using kitty's color capabilities
- GPU-accelerated rendering for TUI applications
