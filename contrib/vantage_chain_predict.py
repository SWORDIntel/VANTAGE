#!/usr/bin/env python3
# vantage_chain_predict.py: Advanced command chain prediction for VANTAGE
# Implements predictive command chains using context data and Markov models

# Standard library imports
import os
import json
import time
import random
from pathlib import Path
import importlib.util

# Third-party imports (with robust error handling)
MARKOVIFY_AVAILABLE = False
try:
    import markovify
    MARKOVIFY_AVAILABLE = True
except ImportError:
    print("Warning: markovify not available, some features will be limited")

# Try to import the context module
CONTEXT_AVAILABLE = False
context_module = None

# Path to the context module
VANTAGE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTEXT_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_context.py")

# Try to import the context module
if os.path.exists(CONTEXT_MODULE_PATH):
    try:
        # Dynamic import of the context module
        spec = importlib.util.spec_from_file_location("vantage_context", CONTEXT_MODULE_PATH)
        context_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(context_module)
        CONTEXT_AVAILABLE = True
    except Exception as e:
        print(f"Error loading vantage_context module: {e}")
        CONTEXT_AVAILABLE = False

# Constants
CHAIN_DIR = os.path.expanduser("~/chains")
CHAIN_MODEL_FILE = os.path.join(CHAIN_DIR, "command_chains.json")
CHAIN_STATS_FILE = os.path.join(CHAIN_DIR, "chain_stats.json")
TASK_CHAINS_FILE = os.path.join(CHAIN_DIR, "task_chains.json")
ERROR_PATTERNS_FILE = os.path.join(CHAIN_DIR, "error_patterns.json")

# Ensure directories exist
Path(CHAIN_DIR).mkdir(parents=True, exist_ok=True)


class ChainModel:
    """Advanced chain prediction model based on multiple algorithms"""

    def __init__(self):
        self.markov_model = None
        self.chain_stats = self._load_chain_stats()
        self.task_chains = self._load_task_chains()
        self.error_patterns = self._load_error_patterns()
        self.last_commands = []
        self.load_model()

    def _load_chain_stats(self):
        """Load command chain statistics"""
        if os.path.exists(CHAIN_STATS_FILE):
            try:
                with open(CHAIN_STATS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"transitions": {}, "last_updated": time.time()}
        return {"transitions": {}, "last_updated": time.time()}

    def _load_task_chains(self):
        """Load task-specific command chains"""
        if os.path.exists(TASK_CHAINS_FILE):
            try:
                with open(TASK_CHAINS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"tasks": {}, "last_updated": time.time()}
        return {"tasks": {}, "last_updated": time.time()}

    def _load_error_patterns(self):
        """Load error correction patterns"""
        if os.path.exists(ERROR_PATTERNS_FILE):
            try:
                with open(ERROR_PATTERNS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"patterns": {}, "last_updated": time.time()}
        return {"patterns": {}, "last_updated": time.time()}

    def load_model(self):
        """Load the markov chain model for command prediction"""
        if not MARKOVIFY_AVAILABLE:
            return False

        if os.path.exists(CHAIN_MODEL_FILE):
            try:
                with open(CHAIN_MODEL_FILE, "r") as f:
                    model_json = f.read()
                self.markov_model = markovify.Text.from_json(model_json)
                return True
            except Exception as e:
                print(f"Error loading chain model: {e}")
                self.markov_model = None
                return False
        return False

    def train_model(self, command_history=None):
        """Train the chain prediction model from command history"""
        if not MARKOVIFY_AVAILABLE:
            return False

        # If command history is not provided, try to get it from context
        if not command_history and CONTEXT_AVAILABLE:
            try:
                context_data = context_module.get_context().get_current_context()
                command_history = context_data["base_context"]["command_history"]
                if isinstance(command_history[0], dict):
                    # Extract just the commands from the history
                    command_history = [
                        cmd.get(
                            "command",
                            "") for cmd in command_history if cmd.get(
                            "exit_code",
                            0) == 0]
            except Exception as e:
                print(f"Error getting command history from context: {e}")
                return False

        if not command_history:
            # Try to read from bash history as a fallback
            try:
                history_file = os.path.expanduser("~/.bash_history")
                if os.path.exists(history_file):
                    with open(history_file, "r") as f:
                        command_history = [line.strip() for line in f.readlines() if line.strip()]
            except Exception as e:
                print(f"Error reading bash history: {e}")
                return False

        if not command_history or len(command_history) < 10:
            print("Insufficient command history for training")
            return False

        # Create a text corpus for the markov model
        # We'll join commands with a special delimiter to model command sequences
        command_text = "\n".join(command_history)

        # Train the model
        self.markov_model = markovify.Text(command_text, state_size=2)

        # Save the model
        with open(CHAIN_MODEL_FILE, "w") as f:
            f.write(self.markov_model.to_json())

        return True

    def update_chain_stats(self, previous_cmd, current_cmd, exit_code=0):
        """Update transition statistics between commands"""
        if not previous_cmd or not current_cmd:
            return

        # Get base commands (first word)
        prev_base = previous_cmd.split()[0]
        curr_base = current_cmd.split()[0]

        # Update transitions
        if prev_base not in self.chain_stats["transitions"]:
            self.chain_stats["transitions"][prev_base] = {}

        if curr_base not in self.chain_stats["transitions"][prev_base]:
            self.chain_stats["transitions"][prev_base][curr_base] = {
                "count": 0,
                "success_count": 0,
                "fail_count": 0,
                "full_examples": []
            }

        # Update counts
        self.chain_stats["transitions"][prev_base][curr_base]["count"] += 1
        if exit_code == 0:
            self.chain_stats["transitions"][prev_base][curr_base]["success_count"] += 1
        else:
            self.chain_stats["transitions"][prev_base][curr_base]["fail_count"] += 1

        # Store full example if we don't have too many
        examples = self.chain_stats["transitions"][prev_base][curr_base]["full_examples"]
        if len(examples) < 5:
            examples.append({
                "from": previous_cmd,
                "to": current_cmd,
                "timestamp": time.time()
            })
        elif random.random() < 0.2:  # 20% chance to replace an old example
            examples[random.randint(0, 4)] = {
                "from": previous_cmd,
                "to": current_cmd,
                "timestamp": time.time()
            }

        self.chain_stats["last_updated"] = time.time()
        self._save_chain_stats()

    def update_task_chains(self, task, commands):
        """Update task-specific command chains"""
        if not task or not commands or len(commands) < 2:
            return

        if task not in self.task_chains["tasks"]:
            self.task_chains["tasks"][task] = {
                "chains": [],
                "commands": {},
                "count": 0
            }

        # Update task count
        self.task_chains["tasks"][task]["count"] += 1

        # Extract command bases (first word)
        cmd_bases = [cmd.split()[0] for cmd in commands]

        # Create chain signature
        chain_sig = " → ".join(cmd_bases)

        # Check if this chain exists
        chain_exists = False
        for chain in self.task_chains["tasks"][task]["chains"]:
            if chain["signature"] == chain_sig:
                chain["count"] += 1
                chain["last_used"] = time.time()
                chain_exists = True
                break

        # If not, add a new chain
        if not chain_exists:
            self.task_chains["tasks"][task]["chains"].append({
                "signature": chain_sig,
                "commands": commands,
                "count": 1,
                "first_seen": time.time(),
                "last_used": time.time()
            })

        # Update individual command frequencies for this task
        for cmd in cmd_bases:
            if cmd not in self.task_chains["tasks"][task]["commands"]:
                self.task_chains["tasks"][task]["commands"][cmd] = 0
            self.task_chains["tasks"][task]["commands"][cmd] += 1

        self.task_chains["last_updated"] = time.time()
        self._save_task_chains()

    def update_error_patterns(self, failed_cmd, successful_cmd):
        """Learn from command errors and corrections"""
        if not failed_cmd or not successful_cmd:
            return

        # Simple heuristic: If commands are similar and one failed but another succeeded,
        # it might be an error correction
        failed_base = failed_cmd.split()[0]
        success_base = successful_cmd.split()[0]

        # If the base commands are the same, this might be a parameter fix
        if failed_base == success_base:
            # Create a pattern entry
            if failed_base not in self.error_patterns["patterns"]:
                self.error_patterns["patterns"][failed_base] = []

            # Add to error patterns
            self.error_patterns["patterns"][failed_base].append({
                "failed": failed_cmd,
                "fixed": successful_cmd,
                "timestamp": time.time()
            })

            # Limit to 10 examples per command
            if len(self.error_patterns["patterns"][failed_base]) > 10:
                # Remove oldest
                self.error_patterns["patterns"][failed_base].sort(key=lambda x: x["timestamp"])
                self.error_patterns["patterns"][failed_base] = self.error_patterns["patterns"][failed_base][-10:]

        self.error_patterns["last_updated"] = time.time()
        self._save_error_patterns()

    def process_command(self, command, exit_code=0):
        """Process a command execution and update models"""
        if not command:
            return

        # Update last commands list
        self.last_commands.append((command, exit_code))
        if len(self.last_commands) > 20:
            self.last_commands = self.last_commands[-20:]

        # If we have at least two commands, update chain stats
        if len(self.last_commands) >= 2:
            prev_cmd, prev_code = self.last_commands[-2]
            self.update_chain_stats(prev_cmd, command, exit_code)

        # If the latest command failed and the one before that also failed,
        # check if this is an error correction pattern
        if len(self.last_commands) >= 3 and exit_code == 0:
            prev_cmd, prev_code = self.last_commands[-2]
            if prev_code != 0:
                self.update_error_patterns(prev_cmd, command)

        # Periodically update task chains if context is available
        if CONTEXT_AVAILABLE and random.random() < 0.1:  # 10% chance
            try:
                context_obj = context_module.get_context()
                task = context_obj.task_context.get("current_task")
                if task:
                    # Get recent commands for this task
                    recent_commands = [cmd for cmd, code in self.last_commands[-10:] if code == 0]
                    if len(recent_commands) >= 2:
                        self.update_task_chains(task, recent_commands)
            except Exception as e:
                print(f"Error updating task chains: {e}")

    def predict_next_command(self, current_cmd, task=None, max_suggestions=5):
        """Predict the next command based on current command and optional task"""
        suggestions = []

        # Method 1: Use transition statistics (most reliable)
        base_cmd = current_cmd.split()[0] if current_cmd else ""
        if base_cmd in self.chain_stats["transitions"]:
            transitions = self.chain_stats["transitions"][base_cmd]
            # Sort by count, descending
            sorted_transitions = sorted(
                transitions.items(),
                key=lambda x: x[1]["count"],
                reverse=True
            )

            for next_cmd, stats in sorted_transitions[:max_suggestions]:
                # Get a full example
                example = ""
                if stats["full_examples"]:
                    example = stats["full_examples"][-1]["to"]
                else:
                    example = next_cmd

                suggestions.append({
                    "command": example,
                    "confidence": min(stats["count"] / 10.0, 0.95),
                    "type": "chain_stats",
                    "description": f"Follows {base_cmd} ({stats['count']} times)"
                })

        # Method 2: Use task-specific chains if a task is provided
        if task and task in self.task_chains["tasks"]:
            task_data = self.task_chains["tasks"][task]

            # Get task-specific command frequencies
            for chain in sorted(task_data["chains"], key=lambda x: x["count"], reverse=True)[:3]:
                # Check if the current command is in this chain
                cmd_bases = [cmd.split()[0] for cmd in chain["commands"]]
                if base_cmd in cmd_bases:
                    # Find the next command in the chain
                    try:
                        idx = cmd_bases.index(base_cmd)
                        if idx < len(cmd_bases) - 1:
                            cmd_bases[idx + 1]
                            next_full = chain["commands"][idx + 1]

                            suggestions.append({
                                "command": next_full,
                                "confidence": min(chain["count"] / 5.0, 0.9),
                                "type": "task_chain",
                                "description": f"Next in {task} workflow"
                            })
                    except ValueError:
                        pass

        # Method 3: Use markov model for more creative suggestions
        if MARKOVIFY_AVAILABLE and self.markov_model:
            try:
                markov_suggestions = []
                for _ in range(max_suggestions):
                    sentence = self.markov_model.make_sentence_with_start(current_cmd, strict=False)
                    if sentence and sentence != current_cmd and sentence not in markov_suggestions:
                        markov_suggestions.append(sentence)

                for cmd in markov_suggestions:
                    suggestions.append({
                        "command": cmd,
                        "confidence": 0.6,  # Lower confidence for markov suggestions
                        "type": "markov",
                        "description": "Statistical prediction"
                    })
            except Exception:
                pass

        # If we have context module available, also use its suggestions
        if CONTEXT_AVAILABLE:
            try:
                context_suggestions = context_module.get_command_suggestions(base_cmd, max_suggestions)
                suggestions.extend(context_suggestions)
            except Exception:
                pass

        # Sort by confidence and take top results
        suggestions.sort(key=lambda x: x["confidence"], reverse=True)
        return suggestions[:max_suggestions]

    def predict_error_fix(self, failed_cmd, max_suggestions=3):
        """Predict how to fix a failed command"""
        if not failed_cmd:
            return []

        suggestions = []
        base_cmd = failed_cmd.split()[0]

        # Look for error patterns for this command
        if base_cmd in self.error_patterns["patterns"]:
            patterns = self.error_patterns["patterns"][base_cmd]

            for pattern in patterns:
                failed = pattern["failed"]
                fixed = pattern["fixed"]

                # Simple string similarity score (Jaccard similarity)
                def jaccard_similarity(a, b):
                    a_tokens = set(a.split())
                    b_tokens = set(b.split())
                    intersection = len(a_tokens.intersection(b_tokens))
                    union = len(a_tokens.union(b_tokens))
                    return intersection / union if union > 0 else 0

                similarity = jaccard_similarity(failed_cmd, failed)

                if similarity > 0.5:  # Only suggest if commands are fairly similar
                    suggestions.append({
                        "command": fixed,
                        "confidence": similarity * 0.9,
                        "type": "error_fix",
                        "description": f"Fix for similar error ({similarity:.2f} similarity)"
                    })

        # Add some common error fix patterns
        if "no such file" in failed_cmd.lower() and "cd" in failed_cmd:
            # Missing directory error
            fixed_cmd = failed_cmd.replace("cd ", "mkdir -p ")
            suggestions.append({
                "command": fixed_cmd,
                "confidence": 0.7,
                "type": "error_fix",
                "description": "Create missing directory"
            })

        if "command not found" in failed_cmd.lower():
            # Try to extract the command that wasn't found
            parts = failed_cmd.split()
            if parts:
                cmd = parts[0]
                suggestions.append({
                    "command": f"which {cmd} || apt-get install {cmd}",
                    "confidence": 0.6,
                    "type": "error_fix",
                    "description": "Install missing command"
                })

        # Sort by confidence and take top results
        suggestions.sort(key=lambda x: x["confidence"], reverse=True)
        return suggestions[:max_suggestions]

    def _save_chain_stats(self):
        """Save chain statistics to file"""
        with open(CHAIN_STATS_FILE, "w") as f:
            json.dump(self.chain_stats, f, indent=2)

    def _save_task_chains(self):
        """Save task chains to file"""
        with open(TASK_CHAINS_FILE, "w") as f:
            json.dump(self.task_chains, f, indent=2)

    def _save_error_patterns(self):
        """Save error patterns to file"""
        with open(ERROR_PATTERNS_FILE, "w") as f:
            json.dump(self.error_patterns, f, indent=2)


# Create a global instance
chain_model = ChainModel()


def process_command(command, exit_code=0):
    """Process a command execution"""
    return chain_model.process_command(command, exit_code)


def predict_next_command(current_cmd, task=None, max_suggestions=5):
    """Predict the next command based on current command and optional task"""
    return chain_model.predict_next_command(current_cmd, task, max_suggestions)


def predict_error_fix(failed_cmd, max_suggestions=3):
    """Predict how to fix a failed command"""
    return chain_model.predict_error_fix(failed_cmd, max_suggestions)


def train_model(command_history=None):
    """Train the chain prediction model"""
    return chain_model.train_model(command_history)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="VANTAGE Command Chain Prediction")
    parser.add_argument("--process", help="Process a command execution")
    parser.add_argument("--exit-code", type=int, default=0, help="Exit code of the command")
    parser.add_argument("--predict", help="Predict next command")
    parser.add_argument("--task", help="Task context for prediction")
    parser.add_argument("--fix", help="Predict fix for a failed command")
    parser.add_argument("--train", action="store_true", help="Train the model")
    parser.add_argument("--max", type=int, default=5, help="Maximum number of suggestions")

    args = parser.parse_args()

    if args.process:
        process_command(args.process, args.exit_code)
        print(f"Processed command: {args.process}")

    if args.predict:
        suggestions = predict_next_command(args.predict, args.task, args.max)
        print(json.dumps(suggestions, indent=2))

    if args.fix:
        fixes = predict_error_fix(args.fix, args.max)
        print(json.dumps(fixes, indent=2))

    if args.train:
        success = train_model()
        if success:
            print("Model trained successfully")
        else:
            print("Error training model")
