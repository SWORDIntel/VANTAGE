#!/usr/bin/env python3
# vantage_autolearn.py: ML auto-learning and suggestion system for VANTAGE

# Standard library imports
import os
import sys
import json
from pathlib import Path
import atexit

# Third-party imports (with robust error handling)
has_openvino = False
try:
    import markovify
    has_openvino = False
    try:
        import openvino as ov
        has_openvino = True
        print("OpenVINO detected and enabled for accelerated suggestions")
    except ImportError:
        pass
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install markovify numpy openvino")
    sys.exit(1)

# Constants
HISTORY_FILE = os.path.expanduser("~/logs/command_history")
MODEL_FILE = os.path.expanduser("~/models/command_model.json")
STATS_FILE = os.path.expanduser("~/models/command_stats.json")
OPENVINO_CACHE = os.path.expanduser("~/cache/openvino_cache")

# Ensure directories exist
Path(os.path.dirname(HISTORY_FILE)).mkdir(parents=True, exist_ok=True)
Path(os.path.dirname(MODEL_FILE)).mkdir(parents=True, exist_ok=True)
Path(os.path.dirname(OPENVINO_CACHE)).mkdir(parents=True, exist_ok=True)

# Command frequency tracking
command_stats = {}
if os.path.exists(STATS_FILE):
    try:
        with open(STATS_FILE, 'r') as f:
            command_stats = json.load(f)
    except BaseException:
        pass


def save_stats():
    try:
        with open(STATS_FILE, 'w') as f:
            json.dump(command_stats, f)
    except Exception as e:
        print(f"Warning: Could not save stats: {e}")


atexit.register(save_stats)


class VantageModel:
    def __init__(self):
        self.markov_model = None
        self.openvino_model = None
        self.load_model()

    def load_model(self):
        # Try to load existing model
        if os.path.exists(MODEL_FILE):
            try:
                with open(MODEL_FILE, 'r') as f:
                    model_json = f.read()
                self.markov_model = markovify.NewlineText.from_json(model_json)
            except Exception as e:
                print(f"Error loading model: {e}")
                self.markov_model = None

        # Initialize OpenVINO if available
        if has_openvino and self.markov_model:
            try:
                self.init_openvino()
            except Exception as e:
                print(f"OpenVINO initialization error: {e}")

    def init_openvino(self):
        if not has_openvino:
            return

        # This is a placeholder for OpenVINO integration
        # In a real implementation, you would convert your model to OpenVINO IR format
        # or use a pre-trained OpenVINO model for text generation
        self.core = ov.Core()
        self.available_devices = self.core.available_devices

        # Use first available hardware accelerator or CPU
        self.device = "CPU"
        for device in self.available_devices:
            if device != "CPU":
                self.device = device
                break

        print(f"Using OpenVINO with device: {self.device}")

    def train(self, text=None):
        """Train or update the model with new text"""
        if not text and os.path.exists(HISTORY_FILE):
            with open(HISTORY_FILE, 'r') as f:
                text = f.read()

        if not text or not text.strip():
            return False

        # Train new model
        new_model = markovify.NewlineText(text, state_size=2)

        # Combine with existing model if available
        if self.markov_model:
            self.markov_model = markovify.combine([self.markov_model, new_model],
                                                  [1.0, 1.0])
        else:
            self.markov_model = new_model

        # Save model
        try:
            with open(MODEL_FILE, 'w') as f:
                f.write(self.markov_model.to_json())
        except Exception as e:
            print(f"Warning: Could not save model: {e}")
            return False

        return True

    def suggest(self, prefix, n=5):
        """Generate suggestions based on prefix"""
        if not self.markov_model:
            return []

        suggestions = []
        try:
            # Try to use OpenVINO for faster inference if available
            if has_openvino and self.openvino_model:
                # Placeholder for OpenVINO inference
                pass

            # Fall back to standard markovify
            for _ in range(n * 3):  # Try more times than needed
                sentence = self.markov_model.make_sentence_with_start(prefix, strict=False)
                if sentence and sentence not in suggestions:
                    suggestions.append(sentence)
                if len(suggestions) >= n:
                    break
        except Exception as e:
            print(f"Error generating suggestions: {e}")

        return suggestions[:n]


def record_command(cmd):
    """Record a command to history file"""
    if not cmd or not cmd.strip():
        return

    # Don't record very short commands or commands that might be passwords
    if len(cmd) < 3 or "password" in cmd.lower() or "secret" in cmd.lower():
        return

    # Update command statistics
    cmd_parts = cmd.split()
    if cmd_parts:
        base_cmd = cmd_parts[0]
        if base_cmd in command_stats:
            command_stats[base_cmd] += 1
        else:
            command_stats[base_cmd] = 1

    # Append to history file
    with open(HISTORY_FILE, 'a+') as f:
        f.write(f"{cmd}\n")


def learn_from_bash_history():
    """Learn from bash history file"""
    bash_history = os.path.expanduser("~/.bash_history")
    if os.path.exists(bash_history):
        with open(bash_history, 'r') as f:
            history = f.read()

        # Append to our history file
        with open(HISTORY_FILE, 'a+') as f:
            for line in history.splitlines():
                line = line.strip()
                if line and len(line) > 3:
                    f.write(f"{line}\n")

        return True
    return False


def setup_bash_hook():
    """Generate bash hook code to be sourced"""
    hook = """
# VANTAGE ML auto-learning hook
PROMPT_COMMAND="history -a; __vantage_record_last_command \\${?} \\${_} \\${BASH_COMMAND}; \\${PROMPT_COMMAND:-:}"

__vantage_record_last_command() {
    local status=$1
    local last_cmd=$(HISTTIMEFORMAT= history 1 | sed 's/^[ 0-9]\\+[ ]\\+//')

    # Only record successful, non-empty commands
    if [ $status -eq 0 ] && [ -n "$last_cmd" ]; then
        python3 "$(dirname "$0")/vantage_autolearn.py" --record "$last_cmd" &>/dev/null &
    fi
}

# VANTAGE ML suggestion function
__vantage_suggest() {
    local prefix="${COMP_WORDS[COMP_CWORD]}"
    if [ ${#prefix} -ge 2 ]; then
        COMPREPLY=( $(python3 "$(dirname "$0")/vantage_autolearn.py" --suggest "$prefix") )
    fi
}

# Register completion for all vantage commands
complete -F __vantage_suggest findlarge find_big_dirs find_recent find_by_ext
"""
    print(hook)


def main():
    import argparse
    parser = argparse.ArgumentParser(description="VANTAGE ML auto-learning and suggestion system")
    parser.add_argument("--record", help="Record a command to history")
    parser.add_argument("--suggest", help="Suggest completions for prefix")
    parser.add_argument("--train", action="store_true", help="Train model from history")
    parser.add_argument("--setup", action="store_true", help="Print bash hook code")
    parser.add_argument("--learn-history", action="store_true", help="Learn from bash history")

    args = parser.parse_args()

    if args.setup:
        setup_bash_hook()
        return

    if args.record:
        record_command(args.record)
        # Periodically retrain model (every ~50 commands based on hash)
        if hash(args.record) % 50 == 0:
            model = VantageModel()
            model.train()
        return

    if args.suggest:
        model = VantageModel()
        for suggestion in model.suggest(args.suggest):
            print(suggestion)
        return

    if args.learn_history:
        learn_from_bash_history()
        print("Learned from bash history")

    if args.train:
        model = VantageModel()
        if model.train():
            print("Model trained successfully")
        else:
            print("No data available for training")


if __name__ == "__main__":
    main()
