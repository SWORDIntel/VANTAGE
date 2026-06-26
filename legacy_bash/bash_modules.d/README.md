# VANTAGE Module Directory

`bash_modules.d/` contains loadable VANTAGE shell modules.

## Rules

- Loadable modules use the `*.module` suffix.
- Helper scripts that are not loadable modules belong in `tools/module_helpers/`.
- Module discovery should not assume arbitrary `*.sh` files in this directory are safe to source.

## Typical Responsibilities

Modules in this directory provide features such as:

- logging and health checks
- module loading and dependency management
- Python/Bash integration
- autocomplete and command prediction
- optional security and workflow helpers

## Working On Modules

Useful local checks from the repository root:

```bash
bash test_module_loading.sh
bash test_clean_module_loading.sh
bash tests/test_parallel_loading.sh
bash tests/test_health_check.sh
```

For broader coverage:

```bash
make test-fast
make test
```
