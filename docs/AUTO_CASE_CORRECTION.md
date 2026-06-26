# Auto Case Correction Module

## Overview

The Auto Case Correction module automatically corrects accidental capitalizations in commands. For example, if you type `GIT PULL` instead of `git pull`, it will automatically execute the correct lowercase version.

## Features

- **Automatic Correction**: Detects and corrects accidentally capitalized commands
- **Smart Detection**: Only corrects commands that are all uppercase (likely accidental)
- **Preserves Intentional Cases**: Leaves mixed-case commands alone
- **Works Everywhere**: Compatible with both kitty and non-kitty pathways
- **Zero Configuration**: Enabled by default, works out of the box

## Examples

```bash
# These will be automatically corrected:
GIT PULL          → git pull
DOCKER PS         → docker ps
KUBECTL GET PODS  → kubectl get pods
NPM INSTALL       → npm install
PYTHON --VERSION  → python --version

# These are preserved (mixed case = intentional):
Git pull          → Git pull (preserved)
DockerPS          → DockerPS (preserved)
```

## How It Works

The module uses `command_not_found_handle` to intercept commands that don't exist and checks if a lowercase version exists. When you type an accidentally capitalized command like `GIT PULL`:

1. Bash first tries to find `GIT` in PATH
2. If `GIT` doesn't exist, `command_not_found_handle` is triggered
3. The module checks if `git` (lowercase) exists
4. If it does, the module executes `git pull` instead
5. The correction happens transparently

**Note**: This works for the common case where accidentally capitalized commands don't exist. If a command with that exact capitalization exists (e.g., a script named `GIT`), bash will execute it directly and the module won't interfere.

## Configuration

### Enable/Disable

The module is enabled by default. To disable it:

```bash
export VANTAGE_AUTO_CASE_CORRECTION_ENABLED=0
```

Then reload your shell configuration:
```bash
source ~/.bashrc
# or
source ~/kitty.rc  # for kitty pathway
```

### Debug Mode

To see when corrections are happening:

```bash
export VANTAGE_DEBUG=1
source ~/.bashrc
```

## Technical Details

### Detection Logic

The module considers a command "accidentally capitalized" if:
1. The command is all uppercase (e.g., `GIT`, `DOCKER`)
2. A lowercase version of the command exists in PATH
3. The command is not already lowercase or mixed case

### Commands Covered

The module works with **any** command in your PATH. It doesn't maintain a hardcoded list - it dynamically checks if a lowercase version exists. This means it works with:
- System commands (`ls`, `cd`, `grep`, etc.)
- Installed tools (`git`, `docker`, `kubectl`, etc.)
- Custom scripts in your PATH
- Any executable in your system

### Performance

The module is lightweight:
- Only activates in interactive shells
- Uses efficient command existence checks
- Minimal overhead on command execution

## Compatibility

- ✅ **Bash 4.0+**: Full support via `command_not_found_handle`
- ✅ **Older Bash**: Falls back to DEBUG trap (may have limitations)
- ✅ **Kitty Pathway**: Fully supported
- ✅ **Non-Kitty Pathway**: Fully supported
- ✅ **Interactive Shells**: Enabled automatically
- ✅ **Non-Interactive Shells**: Disabled automatically

## Troubleshooting

### Module Not Working

1. Check if module is enabled:
   ```bash
   echo $VANTAGE_AUTO_CASE_CORRECTION_ENABLED
   ```
   Should output `1`

2. Check if module is loaded:
   ```bash
   echo $_VANTAGE_AUTO_CASE_CORRECTION_LOADED
   ```
   Should output `1`

3. Verify you're in an interactive shell:
   ```bash
   echo $-
   ```
   Should contain `i`

### Conflicts with Other Handlers

If you have a custom `command_not_found_handle`, the module will:
1. Save your original handler
2. Wrap it with case correction
3. Fall back to your handler if correction doesn't work

### Disabling for Specific Commands

If you need to preserve uppercase for a specific command, you can create an alias:

```bash
alias GIT='GIT'  # Preserves uppercase GIT
```

Or disable the module temporarily:
```bash
VANTAGE_AUTO_CASE_CORRECTION_ENABLED=0 bash
```

## Examples in Action

```bash
# User types:
$ GIT STATUS
# Module corrects to:
$ git status
# Output:
On branch main
Your branch is up to date with 'origin/main'.

# User types:
$ DOCKER PS
# Module corrects to:
$ docker ps
# Output:
CONTAINER ID   IMAGE     COMMAND   CREATED   STATUS   PORTS   NAMES

# User types:
$ KUBECTL GET PODS
# Module corrects to:
$ kubectl get pods
# Output:
NAME    READY   STATUS    RESTARTS   AGE
```

## Best Practices

1. **Let it work**: The module is designed to be invisible - just type naturally
2. **Check output**: If a command seems wrong, check what actually executed
3. **Use aliases**: For commands you want to preserve case, use aliases
4. **Report issues**: If you find commands that should be corrected but aren't, report them

## See Also

- [Kitty Primary CLI Documentation](KITTY_PRIMARY_CLI.md)
- [Module System Documentation](../module_system.md)
