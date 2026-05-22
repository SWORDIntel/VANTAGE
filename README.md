# SENTINEL: Secure ENhanced Terminal INtelligent Layer

SENTINEL is a modular Bash environment focused on security, shell ergonomics, Python integration, and optional ML-assisted workflows.

It builds on ideas from [gitdurandal/bashrc](https://github.com/gitdurandal/bashrc).

## Core Features

- Security-oriented shell modules with integrity and permission hardening
- Dependency-aware module loading with lazy and parallel loading support
- Hybrid autocomplete and command assistance
- Bash/Python integration for config, state, and IPC workflows
- Virtual environment helpers and optional Markov-based text generation

## Quick Start

### Standard install

```bash
git clone https://github.com/SWORDIntel/SENTINEL.git
cd SENTINEL
bash install.sh
source ~/.bashrc
```

### Kitty-first install

```bash
bash install_kitty.sh
```

### Unattended install

```bash
bash install.sh --non-interactive --headless
```

## Validation

SENTINEL ships with a canonical local test runner:

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

- `make test-fast`: core regression suite
- `make test`: broader local validation
- `make test RUN_OPTIONAL=1`: includes optional dependency-gated coverage

Optional Markov dependencies:

```bash
python3 -m pip install -r requirements-markov.txt
```

## Documentation

- Install guide: [`docs/installation.md`](docs/installation.md)
- Documentation index: [`docs/README.md`](docs/README.md)
- Contributing guide: [`CONTRIBUTING.md`](CONTRIBUTING.md)
- Markov generator notes: [`markov_README.md`](markov_README.md)

## Configuration

SENTINEL is configured through shell environment variables plus the YAML config used by the installer.

Common environment variables include:

- `SENTINEL_ROOT`
- `SENTINEL_DATASCIENCE_DIR`
- `PYTHON_INSTALL_DIR`
- `CODE_DIR`
- `OPENVINO_SETUPVARS`
- `ZFS_BUILD_DIR`, `ZFS_AI_DIR`, `ZFS_CODE_DIR`

The installer reads `config.yaml` from the repository root when present and otherwise falls back to `config.yaml.dist`.

## Project Structure

- `bash_modules.d/`: loadable shell modules
- `tools/module_helpers/`: repository helpers that are not loadable modules
- `installer/`: installer implementation and shared installer libraries
- `contrib/`: optional Python-side integrations and utilities
- `scripts/test-all.sh`: canonical local test runner

## Notable Components

### Python integration

`bash_modules.d/python_integration.module` provides:

- `sentinel_config_get` / `sentinel_config_set`
- `sentinel_state_get` / `sentinel_state_set`
- `sentinel_python_exec`
- `sentinel_python_module_install`

### Module system

`module_manager.module` and `bash_modules` provide dependency-aware module loading and module discovery.

### Virtual environments

`bash_functions.d/venv_helpers` provides `mkvenv` for repo-local virtual environment setup.

## Troubleshooting

- Check `~/logs/` for runtime and installer logs.
- Re-run install with `bash install.sh --non-interactive --headless` for a cleaner failure surface.
- Use `make test-fast` before broader debugging.
- Install `requirements-markov.txt` if the optional Markov lane is skipped.

## License

SENTINEL is distributed under the GNU GPL v2. See [`LICENSE`](LICENSE).
