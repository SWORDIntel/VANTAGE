# SENTINEL Contrib Components

The `contrib/` directory contains optional Python utilities that extend the shell-centric core of SENTINEL. These are not the primary installer entrypoints, but several are exercised by the broader test suite and the Python integration bridge.

## What Lives Here

Current contrib scripts include optional helpers for:

- context capture and shell-aware suggestions
- chat and NLU experiments
- command-chain prediction
- GitHub and repository workflow helpers
- OSINT and cyber tooling prototypes
- integration testing for the Bash/Python bridge

Representative files:

- `sentinel_context.py`
- `sentinel_chat.py`
- `sentinel_chain_predict.py`
- `sentinel_gitstar.py`
- `sentinel_osint.py`
- `sentinel_cybersec_ml.py`
- `sentinel_integration_test.py`

## How They Fit Into The Repo

- The shell side lives primarily in `bash_modules.d/python_integration.module`.
- The contrib integration smoke test is `contrib/sentinel_integration_test.py`.
- Broad local validation runs that integration path through `make test`.

## Running Validation

From the repository root:

```bash
make test
```

If you only want to validate the Python bridge path directly:

```bash
bash -lc 'source bash_modules.d/python_integration.module >/dev/null && python3 contrib/sentinel_integration_test.py'
```

## Optional Dependencies

Some contrib-adjacent features, especially Markov generation, require extra packages:

```bash
python3 -m pip install -r requirements-markov.txt
```

Those tests are gated behind:

```bash
make test RUN_OPTIONAL=1
```

## Notes For Contributors

- Treat `contrib/` as optional integration surface, not the core installer contract.
- Keep new scripts import-safe and runnable from the repository root.
- If a script depends on the shell bridge, test it against `python_integration.module` rather than assuming a pre-existing user environment.
