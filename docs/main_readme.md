# SENTINEL Project Overview

This file is a condensed project overview for the `docs/` tree. The canonical user-facing README is the repository root [`README.md`](../README.md).

## What SENTINEL Provides

SENTINEL is a modular Bash environment focused on security, automation, and shell ergonomics. The current codebase centers on:

- dependency-aware shell modules
- installer-managed shell integration
- Python/Bash interoperability helpers
- optional ML and Markov-based features
- regression coverage through a canonical local runner

## Canonical Getting Started

```bash
git clone https://github.com/SWORDIntel/SENTINEL.git
cd SENTINEL
bash install.sh
source ~/.bashrc
```

For unattended setup:

```bash
bash install.sh --non-interactive --headless
```

For kitty-first installs:

```bash
bash install_kitty.sh
```

## Validation

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

Optional Markov dependencies:

```bash
python3 -m pip install -r requirements-markov.txt
```

## Important Layout Conventions

- `bash_modules.d/`: loadable shell modules only
- `tools/module_helpers/`: repository helpers that should not be auto-loaded as modules
- `installer/`: installer implementation and shared installer libraries
- `contrib/`: optional Python helpers and integrations
- `scripts/test-all.sh`: canonical test runner used locally and in CI

## Recommended Reading

- [`installation.md`](installation.md)
- [`architecture.md`](architecture.md)
- [`module_system.md`](module_system.md)
- [`python_ml_components.md`](python_ml_components.md)
- [`contrib_readme.md`](contrib_readme.md)

## Current Maintenance Notes

If you are updating docs, prefer editing the root `README.md` first and keep this file aligned to that baseline rather than expanding a second, divergent README.
