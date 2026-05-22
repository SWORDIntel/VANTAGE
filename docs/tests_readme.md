# SENTINEL Test Scripts

This repository includes shell and Python regression coverage for the installer, module system, Python bridge, and optional Markov path.

## Canonical Entry Points

From the repository root:

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

These targets call [`scripts/test-all.sh`](../scripts/test-all.sh), which provides:

- isolated temporary `HOME`
- broad environment cleanup for test runs
- a stable test order
- optional dependency gating for Markov coverage

## Direct Script Usage

Some scripts are still useful directly during focused debugging:

```bash
bash test_module_loading.sh
bash test_clean_module_loading.sh
bash test_python_integration.sh
bash tests/test_parallel_loading.sh
bash tests/test_health_check.sh
bash tests/test_markov_generator.sh
```

## Guidance

- Prefer the `make` targets for normal validation.
- Use direct shell scripts when iterating on one subsystem.
- Install `requirements-markov.txt` before expecting the optional Markov lane to run.
