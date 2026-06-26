# Kitty Primary CLI Installation Pathway

VANTAGE now supports a dedicated installation pathway that configures **kitty** as the primary terminal CLI. This pathway is optimized for kitty's GPU-accelerated rendering and provides a streamlined experience for users who prefer kitty as their terminal emulator.

## Overview

The Kitty Primary CLI pathway:
- Configures kitty as the primary terminal interface
- Optimizes all VANTAGE modules for kitty's GPU acceleration
- Provides kitty-specific integrations and features
- Skips BLE.sh and Wave Terminal (not needed with kitty)
- Creates a dedicated `kitty.rc` configuration file

## Requirements

- **kitty** terminal emulator installed and available in `PATH`
- A GUI session available (`DISPLAY` or `WAYLAND_DISPLAY` must be set)
- Bash shell (kitty will use bash as the default shell)

## Installation

### Option 1: Direct Kitty Pathway Installer

Run the kitty installer pathway:

```bash
bash install.sh kitty
```

### Option 2: Main Installer with Pathway Selection

Run the main installer and select the kitty pathway when prompted:

```bash
bash installer/install.sh
# Select option 2 when prompted for installation pathway
```

### Option 3: Environment Variable

Set the pathway via environment variable:

```bash
export VANTAGE_INSTALL_PATHWAY=kitty
bash installer/install.sh
```

### Option 4: Command Line Flag

Use the `--kitty-primary` flag:

```bash
bash installer/install.sh --kitty-primary
```

## What Gets Installed

### 1. Kitty Configuration (`~/.config/kitty/kitty.conf`)

The installer creates/updates kitty's configuration with:
- Optimized performance settings for VANTAGE
- VANTAGE-themed color scheme
- Font configuration (JetBrains Mono)
- Shell integration enabled
- Low-latency TUI settings

### 2. Kitty RC File (`~/kitty.rc`)

A dedicated configuration file that:
- Loads all VANTAGE modules optimized for kitty
- Sets kitty-specific environment variables
- Enables GPU acceleration flags
- Configures module system for kitty

### 3. Kitty Startup Script (`~/kitty_startup.sh`)

A startup script that:
- Marks kitty as the primary CLI
- Sources `kitty.rc`
- Loads kitty integration module early
- Sets window titles dynamically

### 4. Vantage Kitty Launcher (`~/.local/bin/vantage-kitty`)

A launcher script that:
- Always runs VANTAGE tools in kitty
- Handles fallback if kitty is unavailable
- Provides consistent kitty experience

### 5. Kitty Integration Module (`bash_modules.d/kitty_integration.module`)

A module that provides:
- `vantage_is_kitty()` - Detect if running in kitty
- `vantage_kitty_available()` - Check if kitty is available
- `vantage_kitty_set_title()` - Set window title
- `vantage_kitty_set_tab_title()` - Set tab title
- `vantage_kitty_enable_features()` - Enable kitty features

## Module Alignment

All VANTAGE modules are automatically aligned to work with kitty:

- **Terminal Detection**: Modules detect kitty via `KITTY_WINDOW_ID` and `TERM` variables
- **GPU Acceleration**: Modules enable GPU acceleration when kitty is detected
- **Color Support**: Full 256-color support optimized for kitty
- **Performance**: Modules use kitty-specific optimizations

## Usage

### Starting Kitty with VANTAGE

Simply start kitty normally. The startup script will automatically:
1. Load `kitty.rc`
2. Initialize VANTAGE modules
3. Set up kitty integration

### Using VANTAGE Tools

All VANTAGE tools work normally. The `vantage-kitty` launcher ensures tools run in kitty:

```bash
vantage-kitty <command>
```

### Checking Kitty Status

Use the kitty integration module functions:

```bash
# Check if running in kitty
vantage_is_kitty && echo "Running in kitty"

# Check if kitty is available
vantage_kitty_available && echo "Kitty is available"

# Set window title
vantage_kitty_set_title "My Project"
```

## Configuration

### Customizing Kitty Config

The VANTAGE-managed block in `~/.config/kitty/kitty.conf` is between:
```
# VANTAGE KITTY BEGIN
...
# VANTAGE KITTY END
```

You can modify settings outside this block, or edit the block directly (it will be preserved on reinstall).

### Customizing Kitty RC

Edit `~/kitty.rc` to customize module loading, environment variables, and aliases specific to kitty.

## Differences from Bash Pathway

| Feature | Bash Pathway | Kitty Pathway |
|---------|-------------|---------------|
| Terminal | Any terminal | kitty only |
| BLE.sh | Optional | Skipped |
| Wave Terminal | Optional | Skipped |
| GPU Acceleration | No | Yes |
| Shell Integration | Basic | Advanced |
| Configuration File | `waveterm.rc` | `kitty.rc` |
| Startup Script | `waveterm.rc` | `kitty_startup.sh` |

## Troubleshooting

### Kitty Not Detected

If kitty is not detected:
1. Ensure kitty is installed: `which kitty`
2. Check GUI session: `echo $DISPLAY` or `echo $WAYLAND_DISPLAY`
3. Verify `KITTY_WINDOW_ID` is set: `echo $KITTY_WINDOW_ID`

### Modules Not Loading

If modules aren't loading:
1. Check `kitty.rc` exists: `ls -la ~/kitty.rc`
2. Verify kitty integration module: `ls -la ~/bash_modules.d/kitty_integration.module`
3. Check startup script: `ls -la ~/kitty_startup.sh`

### Performance Issues

If experiencing performance issues:
1. Verify GPU acceleration: `echo $VANTAGE_KITTY_GPU_ACCEL`
2. Check kitty config: `cat ~/.config/kitty/kitty.conf | grep VANTAGE`
3. Review module loading times: Enable debug mode

## Migration from Bash Pathway

To migrate from the bash pathway to kitty:

1. Backup your current configuration:
   ```bash
   cp ~/.bashrc ~/.bashrc.bak
   cp ~/waveterm.rc ~/waveterm.rc.bak
   ```

2. Run the kitty installer:
   ```bash
   bash install.sh kitty
   ```

3. Restart kitty or source the new configuration:
   ```bash
   source ~/kitty.rc
   ```

## Uninstallation

To remove the kitty pathway:

1. Remove VANTAGE block from `~/.config/kitty/kitty.conf`
2. Remove `~/kitty.rc`
3. Remove `~/kitty_startup.sh`
4. Remove `~/.local/bin/vantage-kitty`
5. Re-run installer with bash pathway

Or simply re-run the bash pathway installer to overwrite.

## Support

For issues specific to the kitty pathway:
- Check logs: `~/logs/install.log`
- Review kitty config: `~/.config/kitty/kitty.conf`
- Verify environment: `env | grep KITTY`
- Test kitty integration: `vantage_is_kitty && echo "OK"`

## See Also

- [Kitty Documentation](https://sw.kovidgoyal.net/kitty/)
- [VANTAGE Module System](../module_system.md)
- [Installation Guide](../README.md)
