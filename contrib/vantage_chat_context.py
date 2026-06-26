#!/usr/bin/env python3
# vantage_chat_context.py - Integration between vantage_chat and the shared context module

# Standard library imports
import os
import importlib.util

# First, try to import the context module
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


def get_enhanced_system_prompt(original_prompt):
    """
    Enhance the system prompt with context information if available
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return original_prompt

    try:
        # Get context formatted for LLM
        context_info = context_module.get_context_for_llm()

        # Create enhanced prompt with context
        enhanced_prompt = f"""{original_prompt}

Current Shell Context:
---------------------
{context_info}
---------------------

When answering questions, consider the above context information when relevant.
Suggest commands that are appropriate to the current task and directory.
If you suggest command sequences, prefer commands that the user has used together before."""

        return enhanced_prompt
    except Exception as e:
        print(f"Error enhancing system prompt with context: {e}")
        return original_prompt


def update_context_after_command(command, exit_code=0):
    """
    Update the shared context after executing a command
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return

    try:
        context_module.record_command(command, exit_code)
    except Exception as e:
        print(f"Error updating context after command: {e}")


def get_command_suggestions(prefix, count=5):
    """
    Get command suggestions based on context
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return []

    try:
        return context_module.get_command_suggestions(prefix, count)
    except Exception as e:
        print(f"Error getting command suggestions: {e}")
        return []


def add_command_sequence(commands):
    """
    Add a command sequence to the context
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return

    try:
        if isinstance(commands, str):
            commands = commands.split(',')
        context_module.add_command_sequence(commands)
    except Exception as e:
        print(f"Error adding command sequence: {e}")


def is_available():
    """
    Check if context integration is available
    """
    return CONTEXT_AVAILABLE


def get_task_context():
    """
    Get the current task context
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return None

    try:
        context = context_module.get_context()
        if hasattr(context, 'task_context') and context.task_context:
            return context.task_context.get("current_task")
        return None
    except Exception as e:
        print(f"Error getting task context: {e}")
        return None


def get_full_context():
    """
    Get the full context object
    """
    if not CONTEXT_AVAILABLE or not context_module:
        return {}

    try:
        return context_module.get_context().get_current_context()
    except Exception as e:
        print(f"Error getting full context: {e}")
        return {}


# If this script is run directly, output status
if __name__ == "__main__":
    print(f"VANTAGE Chat Context Integration Status: {'AVAILABLE' if CONTEXT_AVAILABLE else 'UNAVAILABLE'}")

    if CONTEXT_AVAILABLE:
        # Show current context
        context_text = context_module.get_context_for_llm()
        print("\nCurrent Context:")
        print(context_text)

        # Show example of enhanced prompt
        sample_prompt = "You are VANTAGE, a helpful shell assistant."
        enhanced = get_enhanced_system_prompt(sample_prompt)
        print("\nExample Enhanced Prompt:")
        print(enhanced)

        # Show command suggestions example
        print("\nCommand Suggestions for 'git':")
        suggestions = get_command_suggestions("git")
        for suggestion in suggestions:
            print(f"- {suggestion['command']} ({suggestion['confidence']:.2f}): {suggestion['description']}")
