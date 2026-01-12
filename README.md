# SENTINEL: Secure ENhanced Terminal INtelligent Layer

A hardened, optimized, security-focused shell environment for advanced users, researchers, and security professionals, featuring intelligent context-aware assistance, comprehensive autocomplete, environment management, and cybersecurity capabilities.


Based upon the work of https://github.com/gitdurandal/bashrc ,"A more elegant weapon of a civilized age".

COMING UP:BIG OL' UPDATE w/ an enhanced version of Kitty as who doesnt love cats also like....ill be real i did this before i knew zsh existed...and its kinda lowkey better so instead of conceding ill KICK IT UP A NOTCH(nope no issues hered doc)



## Table of Contents

- [Core Features](#core-features)
- [Installation](#installation)
- [Configuration](#configuration)
- [Features](#features)
  - [Path Management](#path-management)
  - [Python Integration](#python-integration)
  - [Virtual Environments](#virtual-environments)
  - [AI/ML Stack](#aiml-stack)
  - [ZFS Snapshots](#zfs-snapshots)
  - [Module System](#module-system)
  - [Cybersecurity](#cybersecurity)
  - [Script Helper](#script-helper)
- [Usage Examples](#usage-examples)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)
- [Disclaimer](#disclaimer)

## Core Features

- **Comprehensive Security**: HMAC verification for module integrity, permission hardening, and execution sandboxing.
- **Intelligent Command Prediction**: Context-aware suggestions based on history analysis, project context, and statistical modeling.
- **Enhanced Autocomplete**: A hybrid system providing robust autocompletion for commands, arguments, and file paths.
- **Virtual Environment Management**: Automatic Python virtual environment detection, switching, and dependency tracking.
- **Performance Optimization**: Lazy loading, dependency-based module system, and caching to maintain a responsive terminal.

## Installation

SENTINEL supports two installation pathways:

### Bash Pathway (Default)

```bash
# Clone the repository
git clone https://github.com/yourusername/sentinel.git
cd sentinel

# Run the installer
bash installer/install.sh

# Restart your shell or source the configuration
source ~/.bashrc
```

### Kitty Primary CLI Pathway

For users who prefer kitty as their terminal emulator:

```bash
# Clone the repository
git clone https://github.com/yourusername/sentinel.git
cd sentinel

# Run the kitty-specific installer
bash install_kitty.sh

# Or use the main installer and select kitty pathway
bash installer/install.sh
# Select option 2 when prompted
```

The kitty pathway provides GPU-accelerated terminal rendering and optimized module loading. See [Kitty Primary CLI Documentation](docs/KITTY_PRIMARY_CLI.md) for details.

The installer will guide you through the installation process. It will check for dependencies, create the necessary directory structure, and patch your shell configuration files.

## Configuration

SENTINEL is configured through a combination of environment variables and a YAML file.

### Environment Variables

The following environment variables can be set in your `~/.bashrc.postcustom` or other shell configuration files to customize your SENTINEL experience:

-   `SENTINEL_ROOT`: The root directory of your SENTINEL installation. This is set by the installer.
-   `SENTINEL_DATASCIENCE_DIR`: The directory for your data science projects. Defaults to `$HOME/datascience`.
-   `PYTHON_INSTALL_DIR`: Specifies the base directory for Python installations (e.g., `/opt/python`).
-   `CODE_DIR`: Defines the default directory for code projects (e.g., `/opt/code`).
-   `HOMEBREW_PATH`: Path to the Homebrew executable if installed in a non-standard location.
-   `OPENVINO_SETUPVARS`: Path to the OpenVINO `setupvars.sh` script.
-   `C_TOOLCHAIN_PATH`: Base directory for custom C/C++ toolchains.
-   `WAVETERM_PATH`: Path to the Waveterm executable.
-   `ZFS_BUILD_DIR`, `ZFS_AI_DIR`, `ZFS_CODE_DIR`, etc.: Base directories for ZFS-enforced paths, allowing customization of where specific command types are expected to run.

### Configuration File

SENTINEL is configured using a YAML file. By default, it will look for a `config.yaml` file in the root of the repository. If it doesn't find one, it will use the `config.yaml.dist` file as a fallback.

To customize your installation, you can copy `config.yaml.dist` to `config.yaml` and modify it to your needs.

The configuration file allows you to:
-   Enable or disable modules.
-   Configure Python environment settings.
-   Set security verification options.
-   Configure logging levels and locations.

For more information on the available configuration options, please see the `config.yaml.dist` file.

## Optional Integrations

- **Fabric integration (optional)**: see `docs/FABRIC_INTEGRATION.md`
- **Kitty GPU-accelerated terminal path (optional)**: see `docs/KITTY_ACCEL.md`
- **Kitty Primary CLI pathway**: see `docs/KITTY_PRIMARY_CLI.md` - Complete kitty-first installation pathway

## Features

SENTINEL provides a rich set of functions and features to enhance your shell experience.

### Path Management

The `path_manager.sh` script provides a set of functions for managing your shell's `PATH` persistently.

-   `add_path [path]`: Adds a directory to your `PATH` for the current session and saves it to a configuration file to be loaded in future sessions. If no path is provided, it will use the current directory.
-   `remove_path [path]`: Removes a directory from your persistent `PATH` configuration.
-   `list_paths`: Lists all the directories in your persistent `PATH` configuration.
-   `refresh_paths`: Reloads your `PATH` from the persistent configuration file.

### Python Integration

The `python_integration.module` provides a bridge between Bash and Python, with functions for managing state, configuration, and inter-process communication.

-   `sentinel_config_get <key>`: Gets a configuration value.
-   `sentinel_config_set <key> <value>`: Sets a configuration value.
-   `sentinel_state_get <key>`: Gets a state value.
-   `sentinel_state_set <key> <value>`: Sets a state value.
-   `sentinel_python_exec <script> [args...]`: Executes a Python script.
-   `sentinel_python_module_install <module>`: Installs a Python module.
-   `sentinel_python_module_list`: Lists installed Python modules.

### Virtual Environments

The `venv_helpers` script provides a `mkvenv` function for creating Python virtual environments.

-   `mkvenv [directory_name]`: Creates a Python virtual environment in the specified directory (or `.venv` by default) and installs a predefined set of packages.

### AI/ML Stack

SENTINEL includes a set of aliases and functions for working with AI and machine learning tools.

-   `ai-env`: Activates the data science environment.
-   `npu-test`: Runs a test of the NPU.
-   `ai-bench`: Runs a benchmark of the AI stack.
-   `aitest`: Runs a comprehensive test of the AI stack.
-   `aibench`: Runs a benchmark of the AI stack.
-   `datascience`: Activates the data science environment and sets up the necessary environment variables.

### ZFS Snapshots

SENTINEL provides a `zfssnapshot` function for creating ZFS snapshots.

-   `zfssnapshot <snapshot_name_prefix>`: Creates a ZFS snapshot with the given prefix.

### Module System

SENTINEL's functionality is organized into a modular system that allows you to enable or disable features as you see fit. The `module_manager.module` provides a set of functions for managing modules.

-   `module_enable <module_name>`: Enables a module.
-   `module_disable <module_name>`: Disables a module.
-   `module_list`: Lists all available modules and their status.
-   `module_sign <module_name>`: Signs a module with an HMAC signature for integrity verification.

### Cybersecurity

SENTINEL includes a set of modules designed to streamline security operations.

#### AWS Security

The `aws_security.module` provides helper functions and aliases for AWS security operations.

-   `assume_role <role_arn> [role_session_name]`: Assumes an IAM role and exports the temporary credentials.
-   `aws-whoami`: An alias for `aws sts get-caller-identity`.
-   `aws-list-users`: An alias for `aws iam list-users`.
-   `aws-list-roles`: An alias for `aws iam list-roles`.
-   `aws-access-key-summary`: An alias for `aws iam get-account-summary | grep AccessKeys`.

#### Docker Security

The `docker_security.module` provides helper functions for Docker security operations.

-   `scan_image <image_name>`: Scans a Docker image for vulnerabilities using `trivy`.

#### Vault Integration

The `vault_integration.module` provides helper functions for interacting with HashiCorp Vault.

-   `vault_read <secret_path>`: Reads a secret from Vault.
-   `vault_exec <secret_path> <command>`: Wraps a command and injects secrets as environment variables.

### Script Helper

The `script_helper.module` automatically makes scripts executable after they are created or edited. It wraps common text editors (`vim`, `nano`, `code`, `emacs`) and file operations (`cp`, `mv`) to check for a shebang (e.g., `#!/bin/bash`) and make the file executable if it has one.

## Usage Examples

### Create a new Python virtual environment

```bash
mkvenv my_project_env
```

### Add a directory to your PATH

```bash
add_path ~/bin
```

### Run a benchmark of the AI stack

```bash
aibench
```

### Create a ZFS snapshot

```bash
zfssnapshot my_project_
```

## Troubleshooting

-   Check the logs in `~/logs/` for any errors.
-   Run the installer again with the `--non-interactive` flag to see the full output.
-   If you are having issues with a specific module, you can disable it in your `config.yaml` file.

## Contributing

Contributions are welcome! Please see the `CONTRIBUTING.md` file for more information on how to contribute to the project.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This shell environment is highly customized for a specific system and workflow. While it can serve as a template or inspiration, you will likely need to modify the configuration files, scripts, and paths to suit your own needs.

This program was also created before someone introduced me to this "zsh" thing...i thing this is better despite glaring gaps in functionality i will eventually address
