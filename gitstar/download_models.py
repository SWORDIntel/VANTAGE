#!/usr/bin/env python3
"""
VANTAGE Small Model Downloader
-------------------------------
Downloads and sets up small language models for use with VANTAGE.

This script downloads various small (<2GB) language models from Hugging Face
to be used for local embedding, text generation, and other AI tasks.

Usage:
  python3 download_models.py [--model MODEL_NAME] [--all]
  
Examples:
  python3 download_models.py --model phi2-iq3-xs  # Download specific model
  python3 download_models.py --all                # Download all models
"""

import os
import sys
import argparse
import logging
from pathlib import Path
from huggingface_hub import hf_hub_download, HfApi

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)
logger = logging.getLogger("vantage_model_downloader")

# Directory setup
MODEL_DIR = os.path.expanduser("~/Documents/GitHub/VANTAGE/gitstar/models")
Path(MODEL_DIR).mkdir(parents=True, exist_ok=True)

# Define available models with their Hugging Face repo info
AVAILABLE_MODELS = {
    # Phi-2 models (Microsoft)
    "phi2-iq3-xs": {
        "repo_id": "PrunaAI/phi-2-GGUF-smashed",
        "filename": "phi-2.IQ3_XS.gguf",
        "description": "Microsoft Phi-2 (IQ3_XS quantization, ~1.2GB)"
    },
    "phi2-q2k": {
        "repo_id": "PrunaAI/phi-2-GGUF-smashed",
        "filename": "phi-2.Q2_K.gguf",
        "description": "Microsoft Phi-2 (Q2_K quantization, ~1.1GB)"
    },
    
    # Phi-3 mini models (Microsoft)
    "phi3-mini-iq2-xxs": {
        "repo_id": "PrunaAI/Phi-3-mini-128k-instruct-GGUF-Imatrix-smashed",
        "filename": "Phi-3-mini-128k-instruct.IQ2_XXS.gguf",
        "description": "Microsoft Phi-3 Mini (IQ2_XXS quantization, ~1.04GB)"
    },
    "phi3-mini-iq2-xs": {
        "repo_id": "PrunaAI/Phi-3-mini-128k-instruct-GGUF-Imatrix-smashed",
        "filename": "Phi-3-mini-128k-instruct.IQ2_XS.gguf",
        "description": "Microsoft Phi-3 Mini (IQ2_XS quantization, ~1.15GB)"
    },
    
    # Qwen2 0.5B model
    "qwen25-05b": {
        "repo_id": "Qwen/Qwen2.5-0.5B-Instruct-GGUF",
        "filename": "qwen2.5-0.5b-instruct-q4_0.gguf",
        "description": "Alibaba Qwen 2.5 0.5B (Q4_0 quantization, ~550MB)"
    }
}

def list_models():
    """List all available models with descriptions."""
    logger.info("Available models for download:")
    for key, model in AVAILABLE_MODELS.items():
        logger.info(f"  - {key}: {model['description']}")

def download_model(model_key):
    """Download a specific model from Hugging Face."""
    if model_key not in AVAILABLE_MODELS:
        logger.error(f"Model '{model_key}' not found in available models list")
        return False
    
    model = AVAILABLE_MODELS[model_key]
    local_path = os.path.join(MODEL_DIR, model['filename'])
    
    # Skip if already downloaded
    if os.path.exists(local_path):
        logger.info(f"Model {model_key} already downloaded at {local_path}")
        return True
    
    logger.info(f"Downloading {model['description']} from {model['repo_id']}...")
    try:
        hf_hub_download(
            repo_id=model['repo_id'],
            filename=model['filename'],
            local_dir=MODEL_DIR,
            local_dir_use_symlinks=False
        )
        logger.info(f"Successfully downloaded {model_key} to {local_path}")
        return True
    except Exception as e:
        logger.error(f"Error downloading {model_key}: {str(e)}")
        return False

def download_all_models():
    """Download all available models."""
    success = True
    for model_key in AVAILABLE_MODELS:
        if not download_model(model_key):
            success = False
    return success

def main():
    parser = argparse.ArgumentParser(description="Download small language models for VANTAGE")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--model", type=str, help="Specific model to download")
    group.add_argument("--all", action="store_true", help="Download all available models")
    group.add_argument("--list", action="store_true", help="List available models")
    
    args = parser.parse_args()
    
    if args.list:
        list_models()
        return 0
    
    if args.model:
        success = download_model(args.model)
    elif args.all:
        success = download_all_models()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main()) 