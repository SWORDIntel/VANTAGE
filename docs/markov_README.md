# VANTAGE Markov Generator

VANTAGE includes an optional Markov-based text generator implemented in [`contrib/markov_generator.py`](../contrib/markov_generator.py) and exposed through [`bash_modules.d/vantage_markov.module`](../legacy_bash/bash_modules.d/vantage_markov.module).

## Dependencies

The Markov path is optional and requires extra Python packages:

```bash
python3 -m pip install -r requirements.txt
```

## Validation

Run the Markov lane through the canonical test entrypoint:

```bash
make test RUN_OPTIONAL=1
```

Or run the Markov suite directly:

```bash
bash tests/test_markov_generator.sh
```

## Basic Usage

```bash
python3 contrib/markov_generator.py --input input.txt --output output.txt --count 3
python3 contrib/markov_generator.py --stdin --output output.txt < input.txt
python3 contrib/markov_generator.py --corpus-dir ./corpus --output output.txt
```

## Behavior Notes

- Empty output is treated as a failure condition, not a silent success.
- The generator creates its log directory automatically when needed.
- The shell module resolves the generator relative to the repository checkout instead of assuming a fixed path.

## Troubleshooting

- If the optional suite is skipped, install `requirements.txt` first.
- If generation fails on very small corpora, increase input size or reduce constraints such as state size.
- Check `~/logs/markov_generator.log` for runtime diagnostics.
