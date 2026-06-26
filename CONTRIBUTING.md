# Contributing to VANTAGE

## Before You Start

- Search the existing issues: <https://github.com/SWORDIntel/VANTAGE/issues>
- Open a new issue if you are reporting a bug or proposing a material change.
- Keep changes focused. This repository mixes shell, installer, and Python integration code, so broad refactors are harder to review than targeted fixes.

## Development Workflow

1. Fork the repository.
2. Create a descriptive branch.
3. Make your changes.
4. Run the relevant validation locally.
5. Open a pull request against `SWORDIntel/VANTAGE`.

Example branch creation:

```bash
git checkout -b fix/installer-headless-flow
```

## Validation

Use the canonical local runner from the repository root:

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

Use the optional lane only when you have the extra Python packages installed:

```bash
python3 -m pip install -r requirements-markov.txt
```

If you are changing a single subsystem, also run the closest focused test script directly.

## Pull Requests

Good pull requests in this repository usually include:

- a narrow problem statement
- the concrete behavior change
- the validation commands you ran
- notes about any optional dependencies or environment assumptions

Open your pull request here:

- <https://github.com/SWORDIntel/VANTAGE/compare>

## Autocompletion Notes

VANTAGE uses a hybrid completion model:

- [`vantage-completion.bash`](vantage-completion.bash) handles Bash-side completion for the main command surface.
- Python-based tools can use `argcomplete` when appropriate.

If you modify completions, validate the related install and integration paths instead of changing completion logic in isolation.
