#!/usr/bin/env python3
# vantage_chat.py: Context-aware shell assistant powered by local LLMs
# Requires: pip install llama-cpp-python rich readline

# Standard library imports
import os
import sys
import json
import argparse
import subprocess
import hashlib
from pathlib import Path
import shutil
from datetime import datetime
import signal
import hmac
import shlex

# Third-party imports (with robust error handling)
try:
    from llama_cpp import Llama
    from rich.console import Console
    from rich.markdown import Markdown
    from rich.panel import Panel
    from rich.syntax import Syntax
    DEPS_AVAILABLE = True
except ImportError:
    DEPS_AVAILABLE = False

# Constants
MODEL_DIR = os.path.expanduser("~/models")
HISTORY_FILE = os.path.expanduser("~/logs/chat_history.jsonl")
CONFIG_FILE = os.path.expanduser("~/config/chat_config.json")
DEFAULT_MODEL = "mistral-7b-instruct-v0.2.Q4_K_M.gguf"
DEFAULT_MODEL_URL = (
    "https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/"
    "mistral-7b-instruct-v0.2.Q4_K_M.gguf"
)

# HMAC key for secure command execution
EXECUTION_KEY = hashlib.sha256(os.urandom(32)).hexdigest() if not os.path.exists(CONFIG_FILE) else None

# Ensure directories exist
Path(MODEL_DIR).mkdir(parents=True, exist_ok=True)
Path(os.path.dirname(HISTORY_FILE)).mkdir(parents=True, exist_ok=True)
Path(os.path.dirname(CONFIG_FILE)).mkdir(parents=True, exist_ok=True)

# Terminal colors


class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


# Rich console for formatted output
console = Console()


def load_config():
    """Load configuration or create default"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, 'r') as f:
                config = json.load(f)
                return config
        except Exception as e:
            console.print(f"[bold red]Error loading config: {e}[/bold red]")

    # Default config
    config = {
        "model": DEFAULT_MODEL,
        "model_path": os.path.join(MODEL_DIR, DEFAULT_MODEL),
        "context_size": 4096,
        "max_tokens": 2048,
        "temperature": 0.7,
        "execution_key": EXECUTION_KEY or hashlib.sha256(os.urandom(32)).hexdigest(),
        "system_prompt": (
            "You are VANTAGE, a helpful shell assistant that provides concise, accurate answers about bash, Linux commands, "
            "and system administration. When appropriate, provide command examples that the user can run."
        )
    }

    # Save default config
    with open(CONFIG_FILE, 'w') as f:
        json.dump(config, f, indent=2)

    return config


def save_config(config):
    """Save configuration to file"""
    with open(CONFIG_FILE, 'w') as f:
        json.dump(config, f, indent=2)


def download_model(model_url, model_path):
    """Download LLM model if not available"""
    if os.path.exists(model_path):
        return True

    console.print(f"[yellow]Model not found. Downloading from {model_url}...[/yellow]")
    try:
        # Check if wget or curl is available
        if shutil.which("wget"):
            subprocess.run(["wget", model_url, "-O", model_path], check=True)
        elif shutil.which("curl"):
            subprocess.run(["curl", "-L", model_url, "-o", model_path], check=True)
        else:
            console.print("[bold red]Neither wget nor curl found. Please install one to download models.[/bold red]")
            return False

        console.print(f"[green]Model downloaded successfully to {model_path}[/green]")
        return True
    except Exception as e:
        console.print(f"[bold red]Error downloading model: {e}[/bold red]")
        return False


def load_llm(config):
    """Load the LLM model"""
    if not os.path.exists(config["model_path"]):
        success = download_model(DEFAULT_MODEL_URL, config["model_path"])
        if not success:
            return None

    try:
        llm = Llama(
            model_path=config["model_path"],
            n_ctx=config["context_size"],
            n_threads=os.cpu_count() or 4
        )
        return llm
    except Exception as e:
        console.print(f"[bold red]Error loading model: {e}[/bold red]")
        return None


def format_message(role, content):
    """Format messages based on role"""
    if role == "system":
        return f"<s>[INST] <<SYS>>\n{content}\n<</SYS>>\n\n"
    elif role == "user":
        return f"{content} [/INST]\n"
    else:  # assistant
        return f"{content} </s>"


def save_conversation(conversation):
    """Save conversation to history file"""
    entry = {
        "timestamp": datetime.now().isoformat(),
        "conversation": conversation
    }

    # Append to history file
    with open(HISTORY_FILE, 'a') as f:
        f.write(json.dumps(entry) + '\n')


def get_bash_context():
    """Fetch relevant bash context for better responses"""
    context = []

    # Get current directory
    try:
        context.append(f"Current directory: {os.getcwd()}")
    except BaseException:
        pass

    # Recent command history
    try:
        history_path = os.path.expanduser("~/.bash_history")
        if os.path.exists(history_path):
            with open(history_path, 'r') as f:
                recent_commands = f.readlines()[-20:]  # Last 20 commands
                context.append("Recent commands:")
                context.append('\n'.join(cmd.strip() for cmd in recent_commands))
    except BaseException:
        pass

    # Check if we're in a git repo
    try:
        result = subprocess.run(["git", "rev-parse", "--is-inside-work-tree"],
                                capture_output=True, text=True, check=False)
        if result.returncode == 0:
            context.append("In a Git repository")

            # Get git status
            status = subprocess.run(["git", "status", "--short"],
                                    capture_output=True, text=True, check=False)
            if status.stdout:
                context.append("Git status:")
                context.append(status.stdout)
    except BaseException:
        pass

    return "\n".join(context)


def generate_command_signature(command, key):
    """Generate HMAC signature for secure command execution"""
    h = hmac.new(key.encode(), command.encode(), hashlib.sha256)
    return h.hexdigest()


def verify_command_signature(command, signature, key):
    """Verify HMAC signature for secure command execution"""
    expected = generate_command_signature(command, key)
    return hmac.compare_digest(expected, signature)


def execute_command(command, signature, config):
    """Securely execute a shell command with HMAC verification"""
    if not verify_command_signature(command, signature, config["execution_key"]):
        console.print("[bold red]Security error: Command signature verification failed[/bold red]")
        return "Error: Command not executed due to security verification failure"

    try:
        # Parse command safely using shlex
        cmd_parts = shlex.split(command)
        result = subprocess.run(cmd_parts, capture_output=True, text=True)
        output = result.stdout
        error = result.stderr

        if error:
            return f"Error (exit code {result.returncode}):\n{error}\n\nOutput:\n{output}"
        return output or "Command executed successfully (no output)"
    except ValueError as e:
        return f"Error parsing command: {str(e)}"
    except Exception as e:
        return f"Error executing command: {str(e)}"


def chat_loop(llm, config):
    """Main chat loop for interactive mode"""
    console.print(Panel("[bold cyan]VANTAGE Chat Assistant[/bold cyan]\n"
                        "Type your question or command below. Use /help for available commands."))

    # Initialize conversation with system prompt
    conversation = [
        {"role": "system", "content": config["system_prompt"]},
    ]

    # Add bash context to help with answers
    bash_context = get_bash_context()
    if bash_context:
        conversation.append({"role": "system", "content": f"Current shell context:\n{bash_context}"})

    # Main interaction loop
    try:
        while True:
            query = input(f"{Colors.GREEN}> {Colors.ENDC}")

            # Handle special commands
            if query.lower() in ['/exit', '/quit', 'exit', 'quit']:
                break
            elif query.lower() == '/help':
                console.print(Panel("""
[bold]Available Commands:[/bold]
/exit, /quit - Exit the chat
/help - Show this help message
/clear - Clear the current conversation
/save - Save the current conversation
/context - Show the current shell context
/execute <command> - Execute a shell command securely
                """))
                continue
            elif query.lower() == '/clear':
                conversation = [
                    {"role": "system", "content": config["system_prompt"]},
                ]
                console.print("[yellow]Conversation cleared[/yellow]")
                continue
            elif query.lower() == '/save':
                save_conversation(conversation)
                console.print("[green]Conversation saved to history[/green]")
                continue
            elif query.lower() == '/context':
                context = get_bash_context()
                console.print(Syntax(context, "bash"))
                continue
            elif query.lower().startswith('/execute '):
                cmd = query[9:].strip()
                signature = generate_command_signature(cmd, config["execution_key"])
                result = execute_command(cmd, signature, config)
                syntax = Syntax(result, "bash")
                console.print(Panel(syntax, title="Command Result"))
                continue

            # Add user query to conversation
            conversation.append({"role": "user", "content": query})

            # Format prompt for the model
            prompt = ""
            for msg in conversation:
                prompt += format_message(msg["role"], msg["content"])

            # Generate response
            console.print("[italic cyan]Thinking...[/italic cyan]")
            response = llm.create_completion(
                prompt,
                max_tokens=config["max_tokens"],
                temperature=config["temperature"],
                stop=["</s>", "[INST]"],
                echo=False
            )

            # Extract the response
            assistant_message = response["choices"][0]["text"].strip()

            # Display the response as markdown
            console.print(Markdown(assistant_message))

            # Add assistant response to conversation
            conversation.append({"role": "assistant", "content": assistant_message})

    except KeyboardInterrupt:
        console.print("\n[yellow]Chat session ended[/yellow]")

    # Save conversation on exit
    save_conversation(conversation)
    console.print("[green]Conversation saved to history[/green]")


def answer_query(llm, query, config):
    """Answer a single query non-interactively"""
    conversation = [
        {"role": "system", "content": config["system_prompt"]},
    ]

    # Add bash context
    bash_context = get_bash_context()
    if bash_context:
        conversation.append({"role": "system", "content": f"Current shell context:\n{bash_context}"})

    # Add user query
    conversation.append({"role": "user", "content": query})

    # Format prompt
    prompt = ""
    for msg in conversation:
        prompt += format_message(msg["role"], msg["content"])

    # Generate response
    response = llm.create_completion(
        prompt,
        max_tokens=config["max_tokens"],
        temperature=config["temperature"],
        stop=["</s>", "[INST]"],
        echo=False
    )

    # Extract the response
    assistant_message = response["choices"][0]["text"].strip()

    # Display the response as markdown
    console.print(assistant_message)

    # Save to history
    conversation.append({"role": "assistant", "content": assistant_message})
    save_conversation(conversation)


def check_deps():
    """Check if dependencies are installed"""
    if not DEPS_AVAILABLE:
        console.print(Panel(
            "[bold red]Required dependencies are missing.[/bold red]\n\n"
            "Please install the required packages:\n"
            "pip install llama-cpp-python rich readline\n\n"
            "For accelerated inference on NVIDIA GPUs:\n"
            "CMAKE_ARGS=\"-DLLAMA_CUBLAS=on\" pip install llama-cpp-python --force-reinstall --no-cache-dir"
        ))
        return False
    return True


def main():
    """Main function"""
    parser = argparse.ArgumentParser(description="VANTAGE Chat - Context-aware shell assistant")
    parser.add_argument("query", nargs="*", help="Query to ask in non-interactive mode")
    parser.add_argument("--install-deps", action="store_true", help="Install required dependencies")
    parser.add_argument("--model", help="Specify a different model to use")
    parser.add_argument("--model-url", help="URL to download the specified model")

    args = parser.parse_args()

    # Install dependencies if requested
    if args.install_deps:
        subprocess.run([sys.executable, "-m", "pip", "install",
                        "llama-cpp-python", "rich", "readline"])
        console.print("[green]Dependencies installed. Please restart the script.[/green]")
        return

    # Check dependencies
    if not check_deps():
        return

    # Load config
    config = load_config()

    # Override model if specified
    if args.model:
        config["model"] = args.model
        config["model_path"] = os.path.join(MODEL_DIR, args.model)

        if args.model_url:
            download_model(args.model_url, config["model_path"])

        save_config(config)

    # Load LLM
    llm = load_llm(config)
    if not llm:
        return

    # Handle query or interactive mode
    if args.query:
        query = " ".join(args.query)
        answer_query(llm, query, config)
    else:
        chat_loop(llm, config)


if __name__ == "__main__":
    signal.signal(signal.SIGINT, lambda sig, frame: sys.exit(0))
    main()
