# SENTINEL Installer

The supported installer entrypoint is the repository-root [`install.sh`](../install.sh). That wrapper resolves the project root and then delegates to [`installer/install.sh`](install.sh).

## Flow

- `install.sh`: public entrypoint
- `installer/install.sh`: main installer control flow
- `installer/lib/init.sh`: shared initialization and environment setup
- `installer/lib/preflight.sh`: dependency and environment checks
- `installer/lib/install_core.sh`: standard Bash install path
- `installer/lib/install_kitty_core.sh`: kitty-primary install path
- `installer/lib/finalize.sh`: post-install finalization

## Related Components

- [`config.py`](config.py): parses the YAML config and exports environment variables for the installer
- [`bash.sh`](bash.sh): installs shell-facing files and wiring
- [`python.sh`](python.sh): Python environment setup helpers
- [`blesh.sh`](blesh.sh): BLE.sh-related install helpers when that path is enabled

## Supported Modes

Standard install:

```bash
bash install.sh
```

Unattended install:

```bash
bash install.sh --non-interactive --headless
```

Kitty-primary install:

```bash
bash install.sh --kitty-primary
```

## Notes

- Control-flow flags are parsed before any interactive pathway prompt.
- Headless mode disables the interactive pathway selector and skips optional UI-oriented setup.
- The canonical post-change validation path is the repository test runner: `make test-fast` or `make test`.
