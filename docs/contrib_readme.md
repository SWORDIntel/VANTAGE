# SENTINEL Contrib Components

This document mirrors [`contrib/README.md`](../contrib/README.md) and summarizes the optional Python utilities that live under `contrib/`.

## Purpose

The `contrib/` tree holds optional Python helpers and experimental integrations around the Bash-based SENTINEL core. Common themes include:

- context extraction
- chat and suggestion helpers
- command-chain prediction
- OSINT and cyber workflows
- Bash/Python integration validation

## Key Files

- `contrib/sentinel_context.py`
- `contrib/sentinel_chat.py`
- `contrib/sentinel_chain_predict.py`
- `contrib/sentinel_gitstar.py`
- `contrib/sentinel_osint.py`
- `contrib/sentinel_cybersec_ml.py`
- `contrib/sentinel_integration_test.py`

## Validation

Run the broader validation path from the repository root:

```bash
make test
```

To exercise just the integration bridge:

```bash
bash -lc 'source bash_modules.d/python_integration.module >/dev/null && python3 contrib/sentinel_integration_test.py'
```

Optional Markov-related coverage requires:

```bash
python3 -m pip install -r requirements-markov.txt
make test RUN_OPTIONAL=1
```

## Maintenance Guidance

Keep this document aligned with `contrib/README.md`. The root README remains the canonical entrypoint for installation and high-level project usage.
