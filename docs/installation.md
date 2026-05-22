# SENTINEL Installation Guide

This document covers the supported installation, reinstallation, removal, and validation paths for SENTINEL.

## Requirements

- Bash on a Unix-like system
- Python 3
- `git`
- Standard shell tooling used by the installer

Optional features such as the Markov generator require additional Python packages documented in [`requirements-markov.txt`](../requirements-markov.txt).

## Standard Installation

```bash
git clone https://github.com/SWORDIntel/SENTINEL.git
cd SENTINEL
bash install.sh
source ~/.bashrc
```

The installer configures the shell integration, installs the core SENTINEL files, and wires the module system into your shell startup.

## Kitty-Focused Installation

If you use kitty as your primary terminal:

```bash
git clone https://github.com/SWORDIntel/SENTINEL.git
cd SENTINEL
bash install_kitty.sh
```

You can also run `bash install.sh` and choose the kitty pathway when prompted.

## Unattended Installation

For non-interactive environments:

```bash
bash install.sh --non-interactive --headless
```

This path is the right choice for CI, remote bootstrap scripts, or scripted workstation setup.

## Reinstall and Uninstall

Top-level maintenance scripts are included in the repository:

```bash
bash reinstall.sh
bash uninstall.sh
```

Use `reinstall.sh` if the shell wiring is damaged or you want to refresh a local install without manually removing files first.

## Validation

Use the canonical local runner after install:

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

- `make test-fast`: core regression coverage
- `make test`: broader shell and Python integration coverage
- `make test RUN_OPTIONAL=1`: includes optional dependency-gated tests such as the Markov generator

To enable the optional Markov lane:

```bash
python3 -m pip install -r requirements-markov.txt
```

## Repository Layout Notes

- Loadable shell modules live in `bash_modules.d/` and use the `*.module` suffix.
- Repository helper scripts that are not loadable modules live in `tools/module_helpers/`.
- Test orchestration lives in `scripts/test-all.sh` and is exposed through the top-level `Makefile`.

## Troubleshooting

- Check `~/logs/` for runtime and installer logs.
- Re-run the installer with `--non-interactive --headless` to get a cleaner failure surface.
- If optional tests are skipped, install `requirements-markov.txt` before rerunning `make test RUN_OPTIONAL=1`.
- If a specific module is causing trouble, inspect it directly in `bash_modules.d/` and validate module loading with `bash test_module_loading.sh`.
