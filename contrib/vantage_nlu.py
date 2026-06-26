#!/usr/bin/env python3
# vantage_nlu.py: Natural language understanding for VANTAGE
# Translates natural language into shell commands and generates scripts

# Standard library imports
import os
import json
import time
import re
from pathlib import Path
import importlib.util

# Third-party imports (with robust error handling)
LLAMA_AVAILABLE = False
try:
    from llama_cpp import Llama
    LLAMA_AVAILABLE = True
except ImportError:
    print("llama-cpp-python not available, advanced NLU features will be limited")

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

# Try to import the task detection module
TASK_AVAILABLE = False
task_module = None
TASK_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_task_detect.py")

if os.path.exists(TASK_MODULE_PATH):
    try:
        # Dynamic import of the task module
        spec = importlib.util.spec_from_file_location("vantage_task_detect", TASK_MODULE_PATH)
        task_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(task_module)
        TASK_AVAILABLE = True
    except Exception as e:
        print(f"Error loading task detection module: {e}")
        TASK_AVAILABLE = False

# Try to import the chain prediction module
CHAIN_AVAILABLE = False
chain_module = None
CHAIN_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_chain_predict.py")

if os.path.exists(CHAIN_MODULE_PATH):
    try:
        # Dynamic import of the chain module
        spec = importlib.util.spec_from_file_location("vantage_chain_predict", CHAIN_MODULE_PATH)
        chain_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(chain_module)
        CHAIN_AVAILABLE = True
    except Exception as e:
        print(f"Error loading chain prediction module: {e}")
        CHAIN_AVAILABLE = False

# Constants
NLU_DIR = os.path.expanduser("~/nlu")
COMMANDS_DB_FILE = os.path.join(NLU_DIR, "commands_database.json")
PROMPTS_FILE = os.path.join(NLU_DIR, "prompt_templates.json")
TRANSLATIONS_FILE = os.path.join(NLU_DIR, "translations_history.json")
HISTORY_FILE = os.path.join(NLU_DIR, "nlu_history.jsonl")
DEFAULT_MODEL_PATH = os.path.expanduser("~/models/mistral-7b-instruct-v0.2.Q4_K_M.gguf")

# Ensure directories exist
Path(NLU_DIR).mkdir(parents=True, exist_ok=True)


class CommandTranslator:
    """Translates natural language to shell commands using pattern matching and LLMs"""

    def __init__(self):
        self.commands_db = self._load_commands_db()
        self.prompts = self._load_prompts()
        self.translations = self._load_translations()
        self.llm = None
        self.initialize_llm()

    def _load_commands_db(self):
        """Load the command pattern database"""
        if os.path.exists(COMMANDS_DB_FILE):
            try:
                with open(COMMANDS_DB_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return self._create_default_commands_db()
        return self._create_default_commands_db()

    def _load_prompts(self):
        """Load prompt templates"""
        if os.path.exists(PROMPTS_FILE):
            try:
                with open(PROMPTS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return self._create_default_prompts()
        return self._create_default_prompts()

    def _load_translations(self):
        """Load translation history"""
        if os.path.exists(TRANSLATIONS_FILE):
            try:
                with open(TRANSLATIONS_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"history": [], "last_updated": time.time()}
        return {"history": [], "last_updated": time.time()}

    def _create_default_commands_db(self):
        """Create default command patterns for common tasks"""
        return {
            "intent_patterns": {
                "file_search": {
                    "patterns": [
                        "find files", "search for files", "look for files", "locate files"
                    ],
                    "commands": [
                        "find . -name \"{}\" -type f",
                        "find . -iname \"*{}*\" -type f",
                        "grep -r \"{}\" ."
                    ],
                    "examples": [
                        {"query": "find files named config.json", "command": "find . -name \"config.json\" -type f"},
                        {"query": "search for files containing password", "command": "grep -r \"password\" ."}
                    ]
                },
                "process_management": {
                    "patterns": [
                        "list processes", "find process", "kill process", "stop process"
                    ],
                    "commands": [
                        "ps aux | grep {}",
                        "pgrep {}",
                        "kill -9 $(pgrep {})",
                        "pkill {}"
                    ],
                    "examples": [
                        {"query": "list all node processes", "command": "ps aux | grep node"},
                        {"query": "kill firefox process", "command": "pkill firefox"}
                    ]
                },
                "disk_usage": {
                    "patterns": [
                        "disk space", "file size", "large files", "directory size"
                    ],
                    "commands": [
                        "df -h",
                        "du -sh {}",
                        "find . -type f -size +{}M -exec ls -lh {} \\;",
                        "du -h --max-depth=1 {}"
                    ],
                    "examples": [
                        {"query": "check disk space", "command": "df -h"},
                        {"query": "find files larger than 100MB", "command": "find . -type f -size +100M -exec ls -lh {} \\;"}
                    ]
                },
                "archive_extraction": {
                    "patterns": [
                        "extract", "unzip", "uncompress", "untar"
                    ],
                    "commands": [
                        "tar -xf {}",
                        "unzip {}",
                        "tar -xzf {}",
                        "tar -xjf {}"
                    ],
                    "examples": [
                        {"query": "extract archive.tar.gz", "command": "tar -xzf archive.tar.gz"},
                        {"query": "unzip data.zip", "command": "unzip data.zip"}
                    ]
                },
                "network": {
                    "patterns": [
                        "check network", "ping", "network status", "ip address"
                    ],
                    "commands": [
                        "ip addr show",
                        "ping -c 4 {}",
                        "netstat -tuln",
                        "curl ifconfig.me"
                    ],
                    "examples": [
                        {"query": "check my ip address", "command": "curl ifconfig.me"},
                        {"query": "ping google.com", "command": "ping -c 4 google.com"}
                    ]
                },
                "system_info": {
                    "patterns": [
                        "system info", "cpu info", "memory info", "os version"
                    ],
                    "commands": [
                        "uname -a",
                        "cat /etc/os-release",
                        "free -h",
                        "lscpu"
                    ],
                    "examples": [
                        {"query": "show system info", "command": "uname -a"},
                        {"query": "check memory usage", "command": "free -h"}
                    ]
                }
            },
            "parameter_extractors": {
                "filename": {
                    "patterns": [
                        r"(?:named|called)\s+(['\"]?)(\w+[-_.]\w*)\1",
                        r"(?:file|files)\s+(['\"]?)(\w+[-_.]\w*)\1"
                    ]
                },
                "number": {
                    "patterns": [
                        r"(\d+)\s*(?:MB|GB|KB|bytes|megabytes|gigabytes|kilobytes)",
                        r"(\d+)\s*(?:seconds|minutes|hours|days)"
                    ]
                },
                "search_term": {
                    "patterns": [
                        r"containing\s+(['\"]?)([^'\"]+)\1",
                        r"with\s+(['\"]?)([^'\"]+)\1"
                    ]
                }
            },
            "last_updated": time.time()
        }

    def _create_default_prompts(self):
        """Create default prompt templates for LLM command generation"""
        return {
            "command_translation": {
                "system": """You are a helpful shell command assistant that translates natural language descriptions into bash shell commands.
Your goal is to generate the most accurate and efficient command based on the user's request.
Respond with only the command, no explanation or markdown formatting.""",
                "user_template": """Translate the following request into a bash shell command:
Request: {query}

Current directory: {current_dir}
Current OS: {os_info}
{context_info}

Return ONLY the command with no explanations or backticks."""},
            "script_generation": {
                "system": """You are an expert bash script writer. Your task is to create efficient, secure bash scripts based on natural language descriptions.
Follow these guidelines:
1. Always include proper error handling
2. Use secure practices (quote variables, set -e, etc.)
3. Add helpful comments
4. Make scripts portable when possible
5. Focus on readability and maintainability""",
                "user_template": """Write a bash script that accomplishes the following:
Task description: {task_description}

Current environment:
- Directory: {current_dir}
- OS: {os_info}
{context_info}

Please provide ONLY the bash script with appropriate comments."""},
            "command_explanation": {
                "system": """You are a helpful shell assistant that explains shell commands in a clear, concise way.
Your explanations should be informative but brief, focusing on what the command does and any potential risks.""",
                "user_template": """Explain what this shell command does:
```
{command}
```

Please explain:
1. What the command does
2. What each part/flag means
3. Any potential risks or side effects
4. When this command might be useful"""},
            "last_updated": time.time()}

    def initialize_llm(self):
        """Initialize LLM if available"""
        if not LLAMA_AVAILABLE:
            return False

        try:
            if os.path.exists(DEFAULT_MODEL_PATH):
                self.llm = Llama(
                    model_path=DEFAULT_MODEL_PATH,
                    n_ctx=2048,
                    n_threads=os.cpu_count() or 4
                )
                return True
            else:
                print(f"Model not found at {DEFAULT_MODEL_PATH}")
                return False
        except Exception as e:
            print(f"Error initializing LLM: {e}")
            return False

    def save_commands_db(self):
        """Save command database"""
        with open(COMMANDS_DB_FILE, "w") as f:
            json.dump(self.commands_db, f, indent=2)

    def save_prompts(self):
        """Save prompt templates"""
        with open(PROMPTS_FILE, "w") as f:
            json.dump(self.prompts, f, indent=2)

    def save_translations(self):
        """Save translation history"""
        with open(TRANSLATIONS_FILE, "w") as f:
            json.dump(self.translations, f, indent=2)

    def add_to_history(self, query, command, success=None):
        """Add a translation to history"""
        self.translations["history"].append({
            "query": query,
            "command": command,
            "timestamp": time.time(),
            "success": success
        })
        if len(self.translations["history"]) > 100:
            self.translations["history"] = self.translations["history"][-100:]

        self.translations["last_updated"] = time.time()
        self.save_translations()

    def match_intent(self, query):
        """Match a query to command intents using patterns"""
        query = query.lower()
        matched_intents = {}

        # Check each intent pattern
        for intent, data in self.commands_db["intent_patterns"].items():
            for pattern in data["patterns"]:
                if pattern.lower() in query:
                    score = len(pattern) / len(query)  # Simple match score based on length
                    if intent not in matched_intents or score > matched_intents[intent]:
                        matched_intents[intent] = score

        # Return the best matching intent, or None
        if matched_intents:
            best_intent = max(matched_intents.items(), key=lambda x: x[1])
            return best_intent[0], best_intent[1]

        return None, 0.0

    def extract_parameters(self, query):
        """Extract parameters from the query"""
        params = {}

        # For each parameter type, try all its patterns
        for param_type, data in self.commands_db["parameter_extractors"].items():
            for pattern in data["patterns"]:
                matches = re.search(pattern, query, re.IGNORECASE)
                if matches:
                    # If pattern has a captured group, use group 1 or 2
                    if matches.groups():
                        # Group 2 is usually the parameter (group 1 might be quotes)
                        param_value = matches.group(2) if len(matches.groups()) > 1 else matches.group(1)
                        params[param_type] = param_value
                        break

        return params

    def format_command(self, intent, params):
        """Format a command template with extracted parameters"""
        if intent not in self.commands_db["intent_patterns"]:
            return None

        command_templates = self.commands_db["intent_patterns"][intent]["commands"]

        # Try to find a template that can be filled with the extracted parameters
        for template in command_templates:
            try:
                # Simple format string with params
                if params:
                    # For each param, try to use it in the template
                    for param_name, param_value in params.items():
                        if "{}" in template:
                            template = template.replace("{}", param_value, 1)
                        elif "{" + param_name + "}" in template:
                            template = template.replace("{" + param_name + "}", param_value)

                # If there are still unfilled placeholders, skip this template
                if "{}" in template or re.search(r"\{\w+\}", template):
                    continue

                return template
            except (KeyError, IndexError):
                continue

        # If no template can be filled, return the first one as a partial command
        if command_templates:
            return command_templates[0]

        return None

    def translate_with_patterns(self, query):
        """Translate a query to a command using pattern matching"""
        # Match intent
        intent, confidence = self.match_intent(query)
        if not intent:
            return None, 0.0

        # Extract parameters
        params = self.extract_parameters(query)

        # Format command
        command = self.format_command(intent, params)

        return command, confidence

    def translate_with_llm(self, query):
        """Translate a query to a command using LLM"""
        if not LLAMA_AVAILABLE or not self.llm:
            return None, 0.0

        try:
            # Get system context
            context_info = ""
            if CONTEXT_AVAILABLE:
                try:
                    context_info = context_module.get_context_for_llm()
                except Exception:
                    pass

            # Get prompt template
            prompt_template = self.prompts["command_translation"]
            system_prompt = prompt_template["system"]
            user_prompt = prompt_template["user_template"].format(
                query=query,
                current_dir=os.getcwd(),
                os_info=os.uname().sysname + " " + os.uname().release,
                context_info=context_info
            )

            # Format prompt for the model
            prompt = f"<s>[INST] <<SYS>>\n{system_prompt}\n<</SYS>>\n\n{user_prompt} [/INST]"

            # Generate response
            response = self.llm(
                prompt,
                max_tokens=256,
                temperature=0.2,
                top_p=0.9,
                stop=["</s>", "[INST]"],
                echo=False
            )

            if not response or "choices" not in response or not response["choices"]:
                return None, 0.0

            command = response["choices"][0]["text"].strip()

            # Clean up the command (remove any markdown formatting or explanations)
            if command.startswith("```"):
                command = re.sub(r"^```\w*\n", "", command)
                command = re.sub(r"\n```$", "", command)

            # Basic validation - commands shouldn't contain certain syntax or be too long
            if len(command) > 500 or ";" in command or "&" in command or "|" in command:
                # More complex commands need verification
                confidence = 0.6
            else:
                confidence = 0.85

            return command, confidence

        except Exception as e:
            print(f"Error in LLM translation: {e}")
            return None, 0.0

    def translate_query(self, query):
        """Translate a natural language query to a shell command"""
        # Try pattern-based translation first
        pattern_cmd, pattern_confidence = self.translate_with_patterns(query)

        # Try LLM-based translation if available
        llm_cmd, llm_confidence = None, 0.0
        if LLAMA_AVAILABLE and self.llm:
            llm_cmd, llm_confidence = self.translate_with_llm(query)

        # Choose the best translation
        if pattern_confidence > llm_confidence:
            command = pattern_cmd
            confidence = pattern_confidence
            method = "pattern"
        else:
            command = llm_cmd
            confidence = llm_confidence
            method = "llm"

        # If we have a command, add to history
        if command:
            self.add_to_history(query, command)

        return {
            "command": command,
            "confidence": confidence,
            "method": method,
            "query": query
        }

    def generate_script(self, task_description):
        """Generate a bash script from a natural language description"""
        if not LLAMA_AVAILABLE or not self.llm:
            return "Error: LLM not available for script generation"

        try:
            # Get system context
            context_info = ""
            current_task = None

            if CONTEXT_AVAILABLE:
                try:
                    context_info = context_module.get_context_for_llm()
                except Exception:
                    pass

            if TASK_AVAILABLE:
                try:
                    current_task, _, _ = task_module.detect_current_task()
                    if current_task:
                        context_info += f"\nCurrent detected task: {current_task}"
                except Exception:
                    pass

            # Get prompt template
            prompt_template = self.prompts["script_generation"]
            system_prompt = prompt_template["system"]
            user_prompt = prompt_template["user_template"].format(
                task_description=task_description,
                current_dir=os.getcwd(),
                os_info=os.uname().sysname + " " + os.uname().release,
                context_info=context_info
            )

            # Format prompt for the model
            prompt = f"<s>[INST] <<SYS>>\n{system_prompt}\n<</SYS>>\n\n{user_prompt} [/INST]"

            # Generate response
            response = self.llm(
                prompt,
                max_tokens=1024,
                temperature=0.3,
                top_p=0.9,
                stop=["</s>", "[INST]"],
                echo=False
            )

            if not response or "choices" not in response or not response["choices"]:
                return "Error: Script generation failed"

            script = response["choices"][0]["text"].strip()

            # Clean up the script (remove any markdown formatting)
            if script.startswith("```"):
                script = re.sub(r"^```\w*\n", "", script)
                script = re.sub(r"\n```$", "", script)

            # Add shebang if missing
            if not script.startswith("#!/"):
                script = "#!/usr/bin/env bash\n" + script

            return script

        except Exception as e:
            print(f"Error in script generation: {e}")
            return f"Error: {str(e)}"

    def explain_command(self, command):
        """Explain what a shell command does"""
        if not LLAMA_AVAILABLE or not self.llm:
            return "Error: LLM not available for command explanation"

        try:
            # Get prompt template
            prompt_template = self.prompts["command_explanation"]
            system_prompt = prompt_template["system"]
            user_prompt = prompt_template["user_template"].format(command=command)

            # Format prompt for the model
            prompt = f"<s>[INST] <<SYS>>\n{system_prompt}\n<</SYS>>\n\n{user_prompt} [/INST]"

            # Generate response
            response = self.llm(
                prompt,
                max_tokens=512,
                temperature=0.3,
                top_p=0.9,
                stop=["</s>", "[INST]"],
                echo=False
            )

            if not response or "choices" not in response or not response["choices"]:
                return "Error: Command explanation failed"

            explanation = response["choices"][0]["text"].strip()
            return explanation

        except Exception as e:
            print(f"Error in command explanation: {e}")
            return f"Error explaining command: {str(e)}"

    def learn_from_feedback(self, query, command, success):
        """Learn from user feedback about a translation"""
        # Add to history with success/fail info
        self.add_to_history(query, command, success)

        # If it was a successful translation and doesn't match any existing patterns,
        # try to extract a new pattern
        if success and query and command:
            intent, _ = self.match_intent(query)

            if not intent:
                # Try to create a new intent based on the query
                # Extract main action verb
                words = query.lower().split()
                action_verbs = ["find", "search", "list", "show", "get", "check", "create",
                                "delete", "remove", "install", "update", "configure"]

                action = None
                for verb in action_verbs:
                    if verb in words:
                        action = verb
                        break

                if action:
                    # Create a new intent
                    new_intent = f"{action}_command"
                    counter = 1
                    while new_intent in self.commands_db["intent_patterns"]:
                        new_intent = f"{action}_command_{counter}"
                        counter += 1

                    # Add the new intent pattern
                    self.commands_db["intent_patterns"][new_intent] = {
                        "patterns": [query],
                        "commands": [command],
                        "examples": [{"query": query, "command": command}]
                    }

                    self.commands_db["last_updated"] = time.time()
                    self.save_commands_db()
                    return True
            else:
                # Update existing intent
                if command not in self.commands_db["intent_patterns"][intent]["commands"]:
                    self.commands_db["intent_patterns"][intent]["commands"].append(command)
                    self.commands_db["intent_patterns"][intent]["examples"].append({
                        "query": query,
                        "command": command
                    })

                    self.commands_db["last_updated"] = time.time()
                    self.save_commands_db()
                    return True

        return False

    def get_translation_history(self, limit=10):
        """Get recent translation history"""
        history = self.translations["history"]
        return history[-limit:]


# Global instance
translator = CommandTranslator()


def translate_query(query):
    """Translate a natural language query to a shell command"""
    return translator.translate_query(query)


def generate_script(task_description):
    """Generate a bash script from a natural language description"""
    return translator.generate_script(task_description)


def explain_command(command):
    """Explain what a shell command does"""
    return translator.explain_command(command)


def learn_from_feedback(query, command, success):
    """Learn from user feedback about a translation"""
    return translator.learn_from_feedback(query, command, success)


def get_translation_history(limit=10):
    """Get recent translation history"""
    return translator.get_translation_history(limit)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="VANTAGE Natural Language Understanding")
    parser.add_argument("--translate", help="Translate natural language to a command")
    parser.add_argument("--script", help="Generate script from a task description")
    parser.add_argument("--explain", help="Explain what a command does")
    parser.add_argument("--feedback", help="Provide feedback on a translation (format: query|command|success)")
    parser.add_argument("--history", action="store_true", help="Show translation history")

    args = parser.parse_args()

    if args.translate:
        result = translate_query(args.translate)
        print(json.dumps(result, indent=2))

    if args.script:
        script = generate_script(args.script)
        print(script)

    if args.explain:
        explanation = explain_command(args.explain)
        print(explanation)

    if args.feedback:
        parts = args.feedback.split("|")
        if len(parts) == 3:
            query = parts[0]
            command = parts[1]
            success = parts[2].lower() in ["true", "1", "yes", "y"]
            learned = learn_from_feedback(query, command, success)
            print(f"Learning from feedback: {'Success' if learned else 'No changes needed'}")

    if args.history:
        history = get_translation_history()
        print(json.dumps(history, indent=2))
