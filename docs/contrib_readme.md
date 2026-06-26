# VANTAGE Contrib Components

This document mirrors [`contrib/README.md`](../contrib/README.md) and summarizes the optional Python utilities that live under `contrib/`.

## Purpose

The `contrib/` tree holds optional Python helpers and experimental integrations around the Bash-based VANTAGE core. Common themes include:

- context extraction
- chat and suggestion helpers
- command-chain prediction
- OSINT and cyber workflows
- Bash/Python integration validation

## Key Files

- `contrib/vantage_context.py`
- `contrib/vantage_chat.py`
- `contrib/vantage_chain_predict.py`
- `contrib/vantage_gitstar.py`
- `contrib/vantage_osint.py`
- `contrib/vantage_cybersec_ml.py`
- `contrib/vantage_integration_test.py`

## Validation

Run the broader validation path from the repository root:

```bash
make test
```

To exercise just the integration bridge:

```bash
bash -lc 'source bash_modules.d/python_integration.module >/dev/null && python3 contrib/vantage_integration_test.py'
```

Optional Markov-related coverage requires:

```bash
python3 -m pip install -r requirements.txt
make test RUN_OPTIONAL=1
```

## Maintenance Guidance

Keep this document aligned with `contrib/README.md`. The root README remains the canonical entrypoint for installation and high-level project usage.
