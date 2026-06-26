#!/usr/bin/env python3
"""
VANTAGE Small Models Manager
----------------------------
Utility for managing and using small LLMs within VANTAGE.

This script provides functions to:
1. List available downloaded models
2. Run inference on downloaded models using llama.cpp Python bindings
3. Integrate with other VANTAGE components

Usage:
  python3 models_manager.py --list
  python3 models_manager.py --run-model phi3-mini-iq2-xxs --prompt "Write a short summary about cybersecurity"
"""

import os
import sys
import json
import argparse
import logging
from pathlib import Path
import subprocess
import tempfile

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)
logger = logging.getLogger("vantage_models_manager")

# Directory setup
MODEL_DIR = os.path.expanduser("~/Documents/GitHub/VANTAGE/gitstar/models")
Path(MODEL_DIR).mkdir(parents=True, exist_ok=True)

# Model configurations (prompt templates and parameters)
MODEL_CONFIGS = {
    "phi2": {
        "prompt_template": "{prompt}",
        "n_ctx": 4096,
        "n_threads": 4,
        "temperature": 0.7,
        "top_p": 0.9,
        "stop_tokens": []
    },
    "phi3": {
        "prompt_template": "<|user|>\n{prompt}\n<|assistant|>",
        "n_ctx": 8192,
        "n_threads": 4,
        "temperature": 0.7,
        "top_p": 0.9,
        "stop_tokens": ["<|user|>"]
    },
    "qwen": {
        "prompt_template": "<|im_start|>user\n{prompt}<|im_end|>\n<|im_start|>assistant\n",
        "n_ctx": 4096,
        "n_threads": 4,
        "temperature": 0.7,
        "top_p": 0.9,
        "stop_tokens": ["<|im_end|>"]
    }
}

def install_requirements():
    """Install required packages if not already installed."""
    try:
        import llama_cpp
        logger.info("llama_cpp is already installed.")
    except ImportError:
        logger.info("Installing llama_cpp-python...")
        subprocess.run([sys.executable, "-m", "pip", "install", "llama-cpp-python"])
        logger.info("Installed llama_cpp-python successfully.")

def get_available_models():
    """Get list of available downloaded models."""
    models = []
    for item in os.listdir(MODEL_DIR):
        if item.endswith(".gguf"):
            models.append({
                "filename": item,
                "path": os.path.join(MODEL_DIR, item),
                "size_mb": os.path.getsize(os.path.join(MODEL_DIR, item)) / (1024 * 1024)
            })
    return models

def list_models():
    """List all available models."""
    models = get_available_models()
    if not models:
        logger.info("No models found in %s", MODEL_DIR)
        logger.info("Run download_models.py first to download models.")
        return
    
    logger.info("Available models:")
    for i, model in enumerate(models):
        logger.info(f"  {i+1}. {model['filename']} ({model['size_mb']:.2f} MB)")

def get_model_family(model_path):
    """Determine model family from filename."""
    filename = os.path.basename(model_path).lower()
    if "phi-2" in filename:
        return "phi2"
    elif "phi-3" in filename:
        return "phi3"
    elif "qwen" in filename:
        return "qwen"
    else:
        return "phi2"  # default

def run_inference(model_path, prompt, max_tokens=512, temperature=None, n_threads=None):
    """Run inference on a model."""
    try:
        # Import here to allow for installation if needed
        from llama_cpp import Llama
        
        # Determine model family and get appropriate config
        model_family = get_model_family(model_path)
        config = MODEL_CONFIGS.get(model_family, MODEL_CONFIGS["phi2"])
        
        # Format prompt with the appropriate template
        formatted_prompt = config["prompt_template"].format(prompt=prompt)
        
        # Override defaults if specified
        if temperature is None:
            temperature = config["temperature"]
        if n_threads is None:
            n_threads = config["n_threads"]
        
        # Initialize model
        logger.info(f"Loading model: {model_path}")
        llm = Llama(
            model_path=model_path,
            n_ctx=config["n_ctx"],
            n_threads=n_threads
        )
        
        # Run inference
        logger.info("Running inference...")
        output = llm(
            formatted_prompt,
            max_tokens=max_tokens,
            temperature=temperature,
            top_p=config["top_p"],
            echo=False
        )
        
        return output["choices"][0]["text"].strip()
    
    except Exception as e:
        logger.error(f"Error running inference: {str(e)}")
        return None

def generate_text(model_name, prompt, max_tokens=512):
    """Generate text using a specified model."""
    models = get_available_models()
    model_path = None
    
    # Map common model names to filenames
    model_name_mapping = {
        "qwen25-05b": "qwen2.5-0.5b-instruct-q4_0.gguf"
    }
    
    # Check if we have a direct mapping for this model name
    if model_name in model_name_mapping:
        mapped_filename = model_name_mapping[model_name]
        for model in models:
            if model["filename"] == mapped_filename:
                model_path = model["path"]
                break
    
    # If no direct mapping or mapping not found, try partial match
    if not model_path:
        for model in models:
            # Try to match by the model name as a substring of the filename
            if model_name.lower() in model["filename"].lower():
                model_path = model["path"]
                break
    
    if not model_path:
        logger.error(f"Model {model_name} not found. Run --list to see available models.")
        return None
    
    return run_inference(model_path, prompt, max_tokens)

def main():
    # Make sure requirements are installed
    install_requirements()
    
    parser = argparse.ArgumentParser(description="VANTAGE Small Models Manager")
    
    # Command group
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--list", action="store_true", help="List available models")
    group.add_argument("--run-model", type=str, help="Run inference on a specific model")
    
    # Additional parameters
    parser.add_argument("--prompt", type=str, help="Prompt for text generation")
    parser.add_argument("--max-tokens", type=int, default=512, help="Maximum tokens to generate")
    parser.add_argument("--temperature", type=float, help="Temperature for generation")
    parser.add_argument("--threads", type=int, help="Number of CPU threads to use")
    
    args = parser.parse_args()
    
    if args.list:
        list_models()
        return 0
    
    if args.run_model:
        if not args.prompt:
            logger.error("--prompt is required when using --run-model")
            return 1
        
        result = generate_text(args.run_model, args.prompt, args.max_tokens)
        if result:
            print("\n--- Model Output ---")
            print(result)
            print("-------------------\n")
            return 0
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 