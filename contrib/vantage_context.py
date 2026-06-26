#!/usr/bin/env python3
# vantage_context.py: Shared context module for VANTAGE ML systems
# Provides a unified context layer between command learning and chat systems

import os
import json
import time
import hashlib
import subprocess
from pathlib import Path
import hmac

# Constants
CONTEXT_DIR = os.path.expanduser("~/context")
CONTEXT_FILE = os.path.join(CONTEXT_DIR, "shared_context.json")
PATTERNS_FILE = os.path.join(CONTEXT_DIR, "command_patterns.json")
TASK_CONTEXT_FILE = os.path.join(CONTEXT_DIR, "task_context.json")
PREFERENCES_FILE = os.path.join(CONTEXT_DIR, "user_preferences.json")

# Ensure directories exist
Path(CONTEXT_DIR).mkdir(parents=True, exist_ok=True)

# Security - HMAC key for context verification
CONTEXT_KEY = os.environ.get("VANTAGE_CONTEXT_KEY")
if not CONTEXT_KEY:
    # Generate a key if not provided
    key_file = os.path.join(CONTEXT_DIR, ".key")
    if os.path.exists(key_file):
        with open(key_file, "r") as f:
            CONTEXT_KEY = f.read().strip()
    else:
        CONTEXT_KEY = hashlib.sha256(os.urandom(32)).hexdigest()
        with open(key_file, "w") as f:
            f.write(CONTEXT_KEY)
        os.chmod(key_file, 0o600)  # Secure permissions


class VantageContext:
    """Shared context manager for VANTAGE ML systems"""

    def __init__(self):
        self.context = self._load_context()
        self.patterns = self._load_patterns()
        self.task_context = self._load_task_context()
        self.preferences = self._load_preferences()

    def _load_context(self):
        """Load the shared context data"""
        if os.path.exists(CONTEXT_FILE):
            try:
                with open(CONTEXT_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return self._create_default_context()
        return self._create_default_context()

    def _load_patterns(self):
        """Load command patterns data"""
        if os.path.exists(PATTERNS_FILE):
            try:
                with open(PATTERNS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"sequences": {}, "chains": {}, "last_updated": time.time()}
        return {"sequences": {}, "chains": {}, "last_updated": time.time()}

    def _load_task_context(self):
        """Load task context data"""
        if os.path.exists(TASK_CONTEXT_FILE):
            try:
                with open(TASK_CONTEXT_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"current_task": None, "recent_tasks": [], "last_updated": time.time()}
        return {"current_task": None, "recent_tasks": [], "last_updated": time.time()}

    def _load_preferences(self):
        """Load user preferences data"""
        if os.path.exists(PREFERENCES_FILE):
            try:
                with open(PREFERENCES_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"command_frequency": {}, "last_updated": time.time()}
        return {"command_frequency": {}, "last_updated": time.time()}

    def _create_default_context(self):
        """Create default context structure"""
        return {
            "shell_info": self._get_shell_info(),
            "environment": self._get_environment_info(),
            "command_history": self._get_recent_commands(),
            "last_updated": time.time()
        }

    def _get_shell_info(self):
        """Get information about the current shell"""
        shell = os.environ.get("SHELL", "unknown")
        terminal = os.environ.get("TERM", "unknown")
        user = os.environ.get("USER", "unknown")
        hostname = subprocess.getoutput("hostname")

        return {
            "shell": shell,
            "terminal": terminal,
            "user": user,
            "hostname": hostname
        }

    def _get_environment_info(self):
        """Get relevant environment information"""
        cwd = os.getcwd()
        home = os.path.expanduser("~")

        # Git repo information if applicable
        git_info = {}
        try:
            if subprocess.run(["git", "rev-parse", "--is-inside-work-tree"],
                              capture_output=True, text=True).returncode == 0:
                git_info["is_git_repo"] = True
                git_info["branch"] = subprocess.getoutput("git branch --show-current")
                git_info["status"] = subprocess.getoutput("git status --porcelain")
                git_info["remote"] = subprocess.getoutput("git remote -v")
        except BaseException:
            git_info["is_git_repo"] = False

        return {
            "cwd": cwd,
            "home": home,
            "git_info": git_info,
            "path": os.environ.get("PATH", "")
        }

    def _get_recent_commands(self, count=20):
        """Get recent command history"""
        history_file = os.path.expanduser("~/.bash_history")
        if os.path.exists(history_file):
            try:
                with open(history_file, "r") as f:
                    lines = f.readlines()
                    return [line.strip() for line in lines[-count:] if line.strip()]
            except BaseException:
                return []
        return []

    def update_context(self):
        """Update the shared context with latest information"""
        self.context["shell_info"] = self._get_shell_info()
        self.context["environment"] = self._get_environment_info()
        self.context["command_history"] = self._get_recent_commands()
        self.context["last_updated"] = time.time()
        self._save_context()

    def update_from_command(self, command, exit_code=0):
        """Update context based on a command execution"""
        # Add to command history
        if "command_history" not in self.context:
            self.context["command_history"] = []

        # Add with timestamp and status
        cmd_entry = {
            "command": command,
            "timestamp": time.time(),
            "exit_code": exit_code
        }

        self.context["command_history"].append(cmd_entry)
        # Keep only the last 100 commands
        if len(self.context["command_history"]) > 100:
            self.context["command_history"] = self.context["command_history"][-100:]

        # Update command frequency in preferences
        cmd_parts = command.split()
        if cmd_parts:
            base_cmd = cmd_parts[0]
            if base_cmd in self.preferences["command_frequency"]:
                self.preferences["command_frequency"][base_cmd] += 1
            else:
                self.preferences["command_frequency"][base_cmd] = 1

        # Update task context if this command matches a known pattern
        self._update_task_context(command)

        # Save all updates
        self._save_all()

    def _update_task_context(self, command):
        """Detect task context from command patterns"""
        # Simple task detection based on command prefixes
        task_patterns = {
            "git": "version control",
            "docker": "container management",
            "kubectl": "kubernetes management",
            "npm": "node.js development",
            "python": "python development",
            "pip": "python package management",
            "make": "build process",
            "gcc": "C/C++ compilation",
            "ssh": "remote access",
            "scp": "file transfer",
            "find": "file search",
            "grep": "text search"
        }

        cmd_parts = command.split()
        if cmd_parts:
            base_cmd = cmd_parts[0]
            if base_cmd in task_patterns:
                current_task = task_patterns[base_cmd]
                self.task_context["current_task"] = current_task

                # Add to recent tasks if not already there
                if current_task not in self.task_context["recent_tasks"]:
                    self.task_context["recent_tasks"].insert(0, current_task)
                    # Keep only 5 recent tasks
                    self.task_context["recent_tasks"] = self.task_context["recent_tasks"][:5]

    def add_command_sequence(self, commands):
        """Add a command sequence to patterns"""
        if not commands or len(commands) < 2:
            return

        # Use tuple of commands as key
        seq_key = " → ".join(cmd.split()[0] for cmd in commands)

        if seq_key in self.patterns["sequences"]:
            self.patterns["sequences"][seq_key]["count"] += 1
            self.patterns["sequences"][seq_key]["last_used"] = time.time()
        else:
            self.patterns["sequences"][seq_key] = {
                "commands": commands,
                "count": 1,
                "first_seen": time.time(),
                "last_used": time.time()
            }

        self.patterns["last_updated"] = time.time()
        self._save_patterns()

    def get_command_suggestions(self, context_query, max_suggestions=5):
        """Get command suggestions based on the context query"""
        suggestions = []

        # Check if query matches beginning of a known sequence
        for seq_key, data in self.patterns["sequences"].items():
            if seq_key.startswith(context_query):
                # Extract the full command from the sequence that would come next
                commands = data["commands"]
                if len(commands) > 1:
                    suggestions.append({
                        "command": commands[1],
                        "confidence": min(data["count"] / 10.0, 0.95),
                        "type": "sequence",
                        "description": f"Next in sequence: {seq_key}"
                    })

        # Add most frequent commands that match the query
        freq_commands = sorted(
            [(k, v) for k, v in self.preferences["command_frequency"].items()
             if k.startswith(context_query)],
            key=lambda x: x[1],
            reverse=True
        )

        for cmd, freq in freq_commands[:max_suggestions]:
            suggestions.append({
                "command": cmd,
                "confidence": min(freq / 100.0, 0.9),
                "type": "frequency",
                "description": f"Used {freq} times before"
            })

        # Sort by confidence and take top results
        suggestions.sort(key=lambda x: x["confidence"], reverse=True)
        return suggestions[:max_suggestions]

    def get_current_context(self):
        """Get the current context as a dictionary"""
        # Combine all contexts
        full_context = {
            "base_context": self.context,
            "task_context": self.task_context,
            "command_patterns": {
                "sequences": list(self.patterns["sequences"].keys())[:10],
                "frequent_commands": sorted(
                    self.preferences["command_frequency"].items(),
                    key=lambda x: x[1],
                    reverse=True
                )[:10]
            }
        }
        return full_context

    def get_context_for_llm(self):
        """Get context formatted for LLM consumption"""
        context = self.get_current_context()

        # Format as a simplified text representation
        sections = []

        # Shell environment
        shell_info = context["base_context"]["shell_info"]
        sections.append(f"User: {shell_info['user']}@{shell_info['hostname']}")
        sections.append(f"Shell: {shell_info['shell']}")

        # Current directory
        env_info = context["base_context"]["environment"]
        sections.append(f"Current directory: {env_info['cwd']}")

        # Git info if available
        git_info = env_info.get("git_info", {})
        if git_info.get("is_git_repo", False):
            sections.append(f"Git branch: {git_info.get('branch', 'unknown')}")
            if git_info.get('status'):
                sections.append("Git status: Changed files detected")

        # Current task
        task = context["task_context"].get("current_task")
        if task:
            sections.append(f"Current task: {task}")

        # Recent commands (up to 5)
        recent_commands = context["base_context"].get("command_history", [])[-5:]
        if recent_commands:
            sections.append("Recent commands:")
            for cmd in recent_commands:
                if isinstance(cmd, dict):
                    command = cmd.get("command", "")
                    status = "✓" if cmd.get("exit_code", 0) == 0 else "✗"
                    sections.append(f"  {status} {command}")
                else:
                    sections.append(f"  {cmd}")

        # Command patterns
        if context["command_patterns"]["frequent_commands"]:
            top_commands = context["command_patterns"]["frequent_commands"][:3]
            sections.append("Most used commands:")
            for cmd, count in top_commands:
                sections.append(f"  {cmd} (used {count} times)")

        return "\n".join(sections)

    def sign_context(self, context_data):
        """Sign context data for security verification"""
        if isinstance(context_data, dict):
            context_data = json.dumps(context_data, sort_keys=True)

        h = hmac.new(CONTEXT_KEY.encode(), context_data.encode(), hashlib.sha256)
        return h.hexdigest()

    def verify_context(self, context_data, signature):
        """Verify context data signature"""
        expected = self.sign_context(context_data)
        return hmac.compare_digest(expected, signature)

    def _save_context(self):
        """Save context to file"""
        with open(CONTEXT_FILE, "w") as f:
            json.dump(self.context, f, indent=2)

    def _save_patterns(self):
        """Save patterns to file"""
        with open(PATTERNS_FILE, "w") as f:
            json.dump(self.patterns, f, indent=2)

    def _save_task_context(self):
        """Save task context to file"""
        with open(TASK_CONTEXT_FILE, "w") as f:
            json.dump(self.task_context, f, indent=2)

    def _save_preferences(self):
        """Save preferences to file"""
        with open(PREFERENCES_FILE, "w") as f:
            json.dump(self.preferences, f, indent=2)

    def _save_all(self):
        """Save all context data"""
        self._save_context()
        self._save_patterns()
        self._save_task_context()
        self._save_preferences()


# Global instance for easier imports
context = VantageContext()


def get_context():
    """Get the global context instance"""
    return context


def update_context():
    """Update the global context"""
    context.update_context()
    return context.get_current_context()


def record_command(command, exit_code=0):
    """Record a command execution to the context"""
    context.update_from_command(command, exit_code)


def add_command_sequence(commands):
    """Add a command sequence to patterns"""
    context.add_command_sequence(commands)


def get_command_suggestions(query, max_suggestions=5):
    """Get command suggestions based on context"""
    return context.get_command_suggestions(query, max_suggestions)


def get_context_for_llm():
    """Get context formatted for LLM consumption"""
    return context.get_context_for_llm()


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="VANTAGE Shared Context Module")
    parser.add_argument("--update", action="store_true", help="Update context")
    parser.add_argument("--get", action="store_true", help="Get current context")
    parser.add_argument("--record", help="Record a command")
    parser.add_argument("--exit-code", type=int, default=0, help="Exit code of command")
    parser.add_argument("--suggest", help="Get command suggestions")
    parser.add_argument("--for-llm", action="store_true", help="Get context for LLM consumption")
    parser.add_argument("--add-sequence", help="Add command sequence (comma-separated)")

    args = parser.parse_args()

    if args.update:
        update_context()
        print("Context updated")

    if args.get:
        print(json.dumps(context.get_current_context(), indent=2))

    if args.record:
        record_command(args.record, args.exit_code)
        print(f"Recorded command: {args.record}")

    if args.suggest:
        suggestions = get_command_suggestions(args.suggest)
        print(json.dumps(suggestions, indent=2))

    if args.for_llm:
        print(get_context_for_llm())

    if args.add_sequence:
        commands = args.add_sequence.split(",")
        add_command_sequence(commands)
        print(f"Added sequence of {len(commands)} commands")
