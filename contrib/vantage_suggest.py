#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
# vantage_suggest.py: ML-powered CLI suggestions
# Requires: pip install markovify argcomplete

# Standard library imports
import os
import sys
import argparse
import argcomplete

# Third-party imports (with robust error handling)
try:
    import markovify
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install markovify")
    sys.exit(1)

# Config files
HISTORY_FILE = os.path.expanduser("~/logs/command_history")
MODEL_FILE = os.path.expanduser("~/models/command_model.json")

# Attempt to load existing model
model = None
if os.path.exists(MODEL_FILE):
    try:
        model_json = open(MODEL_FILE).read()
        model = markovify.NewlineText.from_json(model_json)
    except Exception:
        model = None

# Build model from history if not loaded
if model is None and os.path.exists(HISTORY_FILE):
    text = open(HISTORY_FILE).read()
    if text.strip():
        model = markovify.NewlineText(text, state_size=2)
        with open(MODEL_FILE, 'w') as f:
            f.write(model.to_json())

# Generate suggestions based on current prefix


def suggest(prefix, n=5):
    suggestions = []
    if model:
        for _ in range(n * 2):
            sentence = model.make_sentence_with_start(prefix, strict=False)
            if sentence and sentence not in suggestions:
                suggestions.append(sentence)
            if len(suggestions) >= n:
                break
    return suggestions


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate command suggestions based on history.")
    parser.add_argument(
        "prefix",
        help="The prefix to generate suggestions for."
    )
    parser.add_argument(
        "-n", "--num-suggestions",
        type=int,
        default=5,
        help="Number of suggestions to generate."
    )

    argcomplete.autocomplete(parser)

    try:
        args = parser.parse_args()
        # In case argcomplete exits after printing completions
        if hasattr(args, 'comp_TYPE'):
             sys.exit(0)

        for s in suggest(args.prefix, args.num_suggestions):
            print(s)

    except SystemExit as e:
        # argcomplete can cause a SystemExit, only exit with error if it's not from argcomplete
        if e.code != 0 and not hasattr(args, 'comp_TYPE'):
            raise
    except Exception as e:
        # Handle other potential errors during script execution
        # For this script, actual errors might be rare after arg parsing,
        # but good practice to have a general handler.
        print(f"An error occurred: {e}", file=sys.stderr)
        sys.exit(1)
