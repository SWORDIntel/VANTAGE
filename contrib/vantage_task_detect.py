#!/usr/bin/env python3
# vantage_task_detect.py: Sophisticated task detection for VANTAGE
# Detects user tasks based on command patterns, file context, and project structure

# Standard library imports
import os
import json
import time
from pathlib import Path
from collections import defaultdict
import importlib.util

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

# Try to import chain prediction model
CHAIN_AVAILABLE = False
chain_module = None
CHAIN_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_chain_predict.py")

if os.path.exists(CHAIN_MODULE_PATH):
    try:
        # Dynamic import of the chain prediction module
        spec = importlib.util.spec_from_file_location("vantage_chain_predict", CHAIN_MODULE_PATH)
        chain_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(chain_module)
        CHAIN_AVAILABLE = True
    except Exception as e:
        print(f"Error loading chain prediction module: {e}")
        CHAIN_AVAILABLE = False

# Constants
TASK_DIR = os.path.expanduser("~/tasks")
TASK_DB_FILE = os.path.join(TASK_DIR, "task_database.json")
PROJECT_PROFILES_FILE = os.path.join(TASK_DIR, "project_profiles.json")
TASK_HISTORY_FILE = os.path.join(TASK_DIR, "task_history.json")

# Ensure directories exist
Path(TASK_DIR).mkdir(parents=True, exist_ok=True)


class TaskDetector:
    """Advanced task detection system based on command and file patterns"""

    def __init__(self):
        self.task_db = self._load_task_db()
        self.project_profiles = self._load_project_profiles()
        self.task_history = self._load_task_history()
        self.current_task = None
        self.current_confidence = 0.0
        self.current_project = None

    def _load_task_db(self):
        """Load the task pattern database"""
        if os.path.exists(TASK_DB_FILE):
            try:
                with open(TASK_DB_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return self._create_default_task_db()
        return self._create_default_task_db()

    def _load_project_profiles(self):
        """Load project profile database"""
        if os.path.exists(PROJECT_PROFILES_FILE):
            try:
                with open(PROJECT_PROFILES_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"projects": {}, "last_updated": time.time()}
        return {"projects": {}, "last_updated": time.time()}

    def _load_task_history(self):
        """Load task history database"""
        if os.path.exists(TASK_HISTORY_FILE):
            try:
                with open(TASK_HISTORY_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"history": [], "last_updated": time.time()}
        return {"history": [], "last_updated": time.time()}

    def _create_default_task_db(self):
        """Create a default task pattern database with common development tasks"""
        return {
            "command_patterns": {
                # Version control tasks
                "git_commit": {
                    "commands": ["git add", "git commit", "git push"],
                    "files": [".git/"],
                    "description": "Git commit workflow"
                },
                "git_merge": {
                    "commands": ["git merge", "git pull", "git checkout"],
                    "files": [".git/"],
                    "description": "Git branch merging"
                },

                # Web development tasks
                "web_dev": {
                    "commands": ["npm", "yarn", "webpack", "gulp", "grunt"],
                    "files": ["package.json", "webpack.config.js", "node_modules/"],
                    "description": "Web development"
                },
                "react_dev": {
                    "commands": ["npm start", "npm run build", "npm test"],
                    "files": ["react", "jsx", "tsx", "node_modules/", "package.json"],
                    "description": "React development"
                },

                # Python development tasks
                "python_dev": {
                    "commands": ["python", "pip", "pytest", "flask", "django"],
                    "files": ["requirements.txt", "setup.py", "pyproject.toml", ".py"],
                    "description": "Python development"
                },
                "data_science": {
                    "commands": ["jupyter", "pandas", "numpy", "sklearn", "matplotlib"],
                    "files": [".ipynb", "csv", "numpy", "pandas"],
                    "description": "Data science work"
                },

                # DevOps tasks
                "docker": {
                    "commands": ["docker", "docker-compose", "container", "image"],
                    "files": ["Dockerfile", "docker-compose.yml"],
                    "description": "Docker container work"
                },
                "kubernetes": {
                    "commands": ["kubectl", "k8s", "helm", "minikube"],
                    "files": [".yaml", "kubeconfig", "deployment.yaml"],
                    "description": "Kubernetes management"
                },

                # System administration tasks
                "sysadmin": {
                    "commands": ["systemctl", "service", "apt", "yum", "dnf", "pacman"],
                    "files": ["/etc/", "/var/log/", "syslog"],
                    "description": "System administration"
                },
                "network_admin": {
                    "commands": ["ip", "ifconfig", "ssh", "nmap", "ping", "traceroute"],
                    "files": ["/etc/network/", "hosts", "resolv.conf"],
                    "description": "Network administration"
                },

                # Security tasks
                "security": {
                    "commands": ["ssh-keygen", "gpg", "openssl", "crypt", "hash"],
                    "files": [".key", ".pem", "cert", "ssl"],
                    "description": "Security operations"
                },

                # Database tasks
                "database": {
                    "commands": ["mysql", "psql", "sqlite", "mongo", "redis"],
                    "files": [".sql", ".db", "database", "schema"],
                    "description": "Database management"
                }
            },
            "file_patterns": {
                # Project types based on files
                "python_project": {
                    "files": ["setup.py", "requirements.txt", "pyproject.toml", ".py"],
                    "description": "Python project"
                },
                "node_project": {
                    "files": ["package.json", "node_modules/", ".js", ".ts"],
                    "description": "Node.js project"
                },
                "c_cpp_project": {
                    "files": ["Makefile", "CMakeLists.txt", ".c", ".cpp", ".h", ".hpp"],
                    "description": "C/C++ project"
                },
                "java_project": {
                    "files": ["pom.xml", "build.gradle", ".java", "mvnw"],
                    "description": "Java project"
                },
                "docker_project": {
                    "files": ["Dockerfile", "docker-compose.yml"],
                    "description": "Docker project"
                }
            },
            "last_updated": time.time()
        }

    def save_task_db(self):
        """Save task database to file"""
        with open(TASK_DB_FILE, "w") as f:
            json.dump(self.task_db, f, indent=2)

    def save_project_profiles(self):
        """Save project profiles to file"""
        with open(PROJECT_PROFILES_FILE, "w") as f:
            json.dump(self.project_profiles, f, indent=2)

    def save_task_history(self):
        """Save task history to file"""
        with open(TASK_HISTORY_FILE, "w") as f:
            json.dump(self.task_history, f, indent=2)

    def save_all(self):
        """Save all data to files"""
        self.save_task_db()
        self.save_project_profiles()
        self.save_task_history()

    def detect_project_type(self, directory=None):
        """Detect the type of project in the current or specified directory"""
        if directory is None:
            directory = os.getcwd()

        # Calculate a hash for the directory path to use as an ID
        dir_hash = self._hash_path(directory)

        # Check if we already know this project
        if dir_hash in self.project_profiles["projects"]:
            project = self.project_profiles["projects"][dir_hash]
            # Update access time
            project["last_accessed"] = time.time()
            self.save_project_profiles()
            return project["type"], project["name"], 1.0

        # Scan directory for files to match patterns
        file_counts = defaultdict(int)

        # Walk through directory and look for pattern matches
        for root, dirs, files in os.walk(directory, topdown=True, followlinks=False):
            # Skip hidden directories
            dirs[:] = [d for d in dirs if not d.startswith('.') and d != "node_modules" and d != "venv"]

            # Check all files
            for file in files:
                file_path = os.path.join(root, file)
                rel_path = os.path.relpath(file_path, directory)

                # Check file against patterns
                for project_type, pattern in self.task_db["file_patterns"].items():
                    for file_pattern in pattern["files"]:
                        if self._match_file_pattern(rel_path, file_pattern):
                            file_counts[project_type] += 1

        # Find the project type with the most matches
        if file_counts:
            project_type, count = max(file_counts.items(), key=lambda x: x[1])
            confidence = min(count / 5, 0.95)  # Cap confidence at 0.95

            # Get project name from directory
            project_name = os.path.basename(os.path.abspath(directory))

            # Save this project profile
            self.project_profiles["projects"][dir_hash] = {
                "type": project_type,
                "name": project_name,
                "path": directory,
                "first_seen": time.time(),
                "last_accessed": time.time(),
                "file_matches": dict(file_counts)
            }
            self.save_project_profiles()

            return project_type, project_name, confidence

        return None, os.path.basename(os.path.abspath(directory)), 0.0

    def _hash_path(self, path):
        """Create a stable hash for a directory path"""
        import hashlib
        # Use absolute path to ensure consistency
        abs_path = os.path.abspath(path)
        return hashlib.md5(abs_path.encode()).hexdigest()

    def _match_file_pattern(self, file_path, pattern):
        """Check if a file matches a pattern"""
        if pattern.startswith('.'):
            # It's a file extension pattern
            return file_path.endswith(pattern)
        elif pattern.endswith('/'):
            # It's a directory pattern
            return pattern.rstrip('/') in file_path.split(os.sep)
        else:
            # It's a filename pattern
            return pattern in file_path

    def detect_task_from_commands(self, commands, with_exit_codes=None):
        """Detect the current task based on recent commands"""
        if not commands:
            return None, 0.0

        # Process commands to get just the base commands
        base_commands = []
        for cmd in commands:
            if isinstance(cmd, dict):
                # Handle dictionary format from context module
                base_cmd = cmd.get("command", "").split()[0]
                base_commands.append(base_cmd)
            elif isinstance(cmd, tuple) and len(cmd) == 2:
                # Handle tuple format (command, exit_code)
                base_cmd = cmd[0].split()[0]
                base_commands.append(base_cmd)
            else:
                # Handle simple string format
                base_cmd = cmd.split()[0]
                base_commands.append(base_cmd)

        # Count occurrences of each command pattern
        task_scores = defaultdict(float)

        for task_name, pattern in self.task_db["command_patterns"].items():
            pattern_commands = pattern["commands"]

            # Count how many pattern commands appear in the recent commands
            matches = sum(1 for cmd in base_commands if any(pcmd in cmd for pcmd in pattern_commands))
            if matches > 0:
                # Calculate a score based on the number of matches
                # More matches = higher score
                task_scores[task_name] = matches / len(pattern_commands)

        # Find the highest scoring task
        if task_scores:
            task, score = max(task_scores.items(), key=lambda x: x[1])
            # Scale the score to a confidence value
            confidence = min(score, 0.9)  # Cap at 0.9
            return task, confidence

        return None, 0.0

    def detect_task_from_files(self, directory=None):
        """Detect the current task based on files in the current directory"""
        if directory is None:
            directory = os.getcwd()

        # List files in the current directory
        files = []
        for root, dirs, filenames in os.walk(directory, topdown=True):
            # Skip hidden directories and limit depth
            if root.count(os.sep) - directory.count(os.sep) > 2:
                continue
            dirs[:] = [d for d in dirs if not d.startswith('.') and d != "node_modules" and d != "venv"]

            for filename in filenames:
                if not filename.startswith('.'):
                    rel_path = os.path.relpath(os.path.join(root, filename), directory)
                    files.append(rel_path)

        # Score each task pattern based on file matches
        task_scores = defaultdict(float)

        for task_name, pattern in self.task_db["command_patterns"].items():
            if "files" in pattern:
                pattern_files = pattern["files"]

                # Count how many pattern files match
                matches = 0
                for file in files:
                    for pattern_file in pattern_files:
                        if self._match_file_pattern(file, pattern_file):
                            matches += 1
                            break

                if matches > 0:
                    # Calculate a score
                    task_scores[task_name] = matches / len(files)

        # Find the highest scoring task
        if task_scores:
            task, score = max(task_scores.items(), key=lambda x: x[1])
            # Scale the score to a confidence value
            confidence = min(score * 0.8, 0.8)  # Cap at 0.8 for file-based detection
            return task, confidence

        return None, 0.0

    def detect_current_task(self):
        """Detect the current task using all available signals"""
        # First, get the project type
        project_type, project_name, project_confidence = self.detect_project_type()
        self.current_project = project_name if project_confidence > 0.5 else None

        # Start with command-based detection
        commands = []
        command_task = None
        command_confidence = 0.0

        # Try to get commands from context module
        if CONTEXT_AVAILABLE:
            try:
                context_data = context_module.get_context().get_current_context()
                commands = context_data["base_context"]["command_history"]
                if commands:
                    command_task, command_confidence = self.detect_task_from_commands(commands)
            except Exception as e:
                print(f"Error getting commands from context: {e}")

        # Fall back to chain module if available
        elif CHAIN_AVAILABLE:
            try:
                commands = chain_module.chain_model.last_commands
                if commands:
                    command_task, command_confidence = self.detect_task_from_commands(commands)
            except Exception as e:
                print(f"Error getting commands from chain module: {e}")

        # Then try file-based detection
        file_task, file_confidence = self.detect_task_from_files()

        # Combine the signals
        # If command confidence is high, prefer that
        if command_confidence > 0.7:
            task = command_task
            confidence = command_confidence
        # If file confidence is high, use that
        elif file_confidence > 0.6:
            task = file_task
            confidence = file_confidence
        # Otherwise take the highest confidence one
        else:
            if command_confidence >= file_confidence:
                task = command_task
                confidence = command_confidence
            else:
                task = file_task
                confidence = file_confidence

        # Update current task
        old_task = self.current_task
        self.current_task = task
        self.current_confidence = confidence

        # If task changed, add to history
        if old_task != task and task:
            self.task_history["history"].append({
                "task": task,
                "project": self.current_project,
                "confidence": confidence,
                "timestamp": time.time()
            })
            # Keep history at a reasonable size
            if len(self.task_history["history"]) > 100:
                self.task_history["history"] = self.task_history["history"][-100:]

            self.task_history["last_updated"] = time.time()
            self.save_task_history()

            # If we have context module, update it
            if CONTEXT_AVAILABLE:
                try:
                    context_obj = context_module.get_context()
                    context_obj.task_context["current_task"] = task
                    context_obj._save_task_context()
                except Exception as e:
                    print(f"Error updating context with task: {e}")

        return task, confidence, self.current_project

    def get_task_data(self, task_name):
        """Get detailed information about a specific task"""
        if task_name in self.task_db["command_patterns"]:
            return self.task_db["command_patterns"][task_name]
        return None

    def get_project_data(self, project_hash=None):
        """Get detailed information about a specific project or the current project"""
        if project_hash is None:
            current_dir = os.getcwd()
            project_hash = self._hash_path(current_dir)

        if project_hash in self.project_profiles["projects"]:
            return self.project_profiles["projects"][project_hash]
        return None

    def learn_from_commands(self, commands, task_name):
        """Learn new command patterns for a specific task"""
        if not commands or not task_name:
            return False

        # Create task if it doesn't exist
        if task_name not in self.task_db["command_patterns"]:
            self.task_db["command_patterns"][task_name] = {
                "commands": [],
                "files": [],
                "description": f"User-defined task: {task_name}"
            }

        # Extract base commands
        base_commands = [cmd.split()[0] for cmd in commands if cmd]

        # Add new commands to the pattern
        task_pattern = self.task_db["command_patterns"][task_name]
        for cmd in base_commands:
            if cmd and cmd not in task_pattern["commands"]:
                task_pattern["commands"].append(cmd)

        self.task_db["last_updated"] = time.time()
        self.save_task_db()
        return True

    def learn_from_files(self, directory, task_name):
        """Learn new file patterns for a specific task"""
        if not directory or not task_name:
            return False

        # Create task if it doesn't exist
        if task_name not in self.task_db["command_patterns"]:
            self.task_db["command_patterns"][task_name] = {
                "commands": [],
                "files": [],
                "description": f"User-defined task: {task_name}"
            }

        # Find key files in the directory
        key_files = []

        # Look for common project files
        project_files = [
            "package.json", "requirements.txt", "setup.py", "Makefile",
            "CMakeLists.txt", "pom.xml", "build.gradle", "Dockerfile",
            "docker-compose.yml", "pyproject.toml", "Cargo.toml"
        ]

        for file in project_files:
            if os.path.exists(os.path.join(directory, file)):
                key_files.append(file)

        # Look for common directories
        for dirname in os.listdir(directory):
            if os.path.isdir(os.path.join(directory, dirname)) and not dirname.startswith('.'):
                key_files.append(f"{dirname}/")

        # Look for file extensions
        extensions = set()
        for root, dirs, files in os.walk(directory, topdown=True):
            # Skip hidden directories and limit depth
            if root.count(os.sep) - directory.count(os.sep) > 1:
                continue
            dirs[:] = [d for d in dirs if not d.startswith('.')]

            for filename in files:
                if "." in filename and not filename.startswith('.'):
                    ext = os.path.splitext(filename)[1]
                    if ext:
                        extensions.add(ext)

        # Add file extensions to key files (limit to top 3)
        common_extensions = sorted(extensions, key=lambda x: len(x))[:3]
        key_files.extend(common_extensions)

        # Add new files to the pattern
        task_pattern = self.task_db["command_patterns"][task_name]
        for file in key_files:
            if file and file not in task_pattern["files"]:
                task_pattern["files"].append(file)

        self.task_db["last_updated"] = time.time()
        self.save_task_db()
        return True

    def get_task_history(self, limit=10):
        """Get recent task history"""
        history = self.task_history["history"]
        return history[-limit:]

    def get_task_suggestions(self, current_directory=None):
        """Get suggested tasks based on the current directory"""
        if current_directory is None:
            current_directory = os.getcwd()

        # Detect project type
        project_type, _, _ = self.detect_project_type(current_directory)

        suggestions = []

        # Suggest based on project type
        if project_type:
            # Find tasks that match this project type
            for task_name, pattern in self.task_db["command_patterns"].items():
                # Check if any file patterns match
                matches = False
                if "files" in pattern:
                    for file_pattern in pattern["files"]:
                        if file_pattern in project_type:
                            matches = True
                            break

                if matches:
                    suggestions.append({
                        "task": task_name,
                        "description": pattern.get("description", ""),
                        "confidence": 0.7,
                        "reason": f"Matches {project_type} project files"
                    })

        # Suggest based on recent history
        recent_tasks = {}
        for entry in reversed(self.task_history["history"]):
            task = entry["task"]
            if task not in recent_tasks and len(recent_tasks) < 3:
                recent_tasks[task] = {
                    "task": task,
                    "description": self.task_db["command_patterns"].get(task, {}).get("description", ""),
                    "confidence": 0.5,
                    "reason": "Recently performed task"
                }

        # Add recent tasks to suggestions
        suggestions.extend(recent_tasks.values())

        # Sort by confidence
        suggestions.sort(key=lambda x: x["confidence"], reverse=True)
        return suggestions


# Global instance
task_detector = TaskDetector()


def detect_current_task():
    """Detect the current task"""
    return task_detector.detect_current_task()


def get_task_data(task_name):
    """Get detailed information about a task"""
    return task_detector.get_task_data(task_name)


def get_project_data(project_hash=None):
    """Get project information"""
    return task_detector.get_project_data(project_hash)


def learn_from_commands(commands, task_name):
    """Learn new command patterns for a task"""
    return task_detector.learn_from_commands(commands, task_name)


def learn_from_files(directory, task_name):
    """Learn new file patterns for a task"""
    return task_detector.learn_from_files(directory, task_name)


def get_task_history(limit=10):
    """Get task history"""
    return task_detector.get_task_history(limit)


def get_task_suggestions():
    """Get task suggestions"""
    return task_detector.get_task_suggestions()


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="VANTAGE Task Detection System")
    parser.add_argument("--detect", action="store_true", help="Detect current task")
    parser.add_argument("--project", action="store_true", help="Detect project type")
    parser.add_argument("--task-info", help="Get task information")
    parser.add_argument("--project-info", help="Get project information")
    parser.add_argument("--learn-commands", help="Learn commands for a task (format: task_name,cmd1,cmd2,...)")
    parser.add_argument("--learn-files", help="Learn files for a task (format: task_name,directory)")
    parser.add_argument("--history", action="store_true", help="Show task history")
    parser.add_argument("--suggestions", action="store_true", help="Get task suggestions")

    args = parser.parse_args()

    if args.detect:
        task, confidence, project = detect_current_task()
        print(json.dumps({
            "task": task,
            "confidence": confidence,
            "project": project
        }, indent=2))

    if args.project:
        project_type, project_name, confidence = task_detector.detect_project_type()
        print(json.dumps({
            "type": project_type,
            "name": project_name,
            "confidence": confidence
        }, indent=2))

    if args.task_info:
        task_data = get_task_data(args.task_info)
        print(json.dumps(task_data, indent=2))

    if args.project_info:
        project_data = get_project_data(args.project_info)
        print(json.dumps(project_data, indent=2))

    if args.learn_commands:
        parts = args.learn_commands.split(",")
        if len(parts) >= 2:
            task_name = parts[0]
            commands = parts[1:]
            success = learn_from_commands(commands, task_name)
            print(f"Learning commands for {task_name}: {'Success' if success else 'Failed'}")

    if args.learn_files:
        parts = args.learn_files.split(",")
        if len(parts) == 2:
            task_name = parts[0]
            directory = parts[1]
            success = learn_from_files(directory, task_name)
            print(f"Learning files for {task_name}: {'Success' if success else 'Failed'}")

    if args.history:
        history = get_task_history()
        print(json.dumps(history, indent=2))

    if args.suggestions:
        suggestions = get_task_suggestions()
        print(json.dumps(suggestions, indent=2))
