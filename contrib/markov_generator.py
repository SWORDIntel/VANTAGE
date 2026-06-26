#!/usr/bin/env python3
"""
VANTAGE Markov Text Generator
------------------------------
A text generation utility that creates natural-sounding text based on input sources.
Uses the Markovify library for state chain text generation with customizable parameters.

Features:
- Multiple input sources (files, strings, stdin)
- Adjustable state size for different coherence levels
- Output formatting options
- Secure file handling with validation
- Progress tracking for large inputs
- Comprehensive logging

Usage:
    ./markov_generator.py --input input.txt --output output.txt --state-size 2 --count 10
    cat input.txt | ./markov_generator.py --state-size 3 --count 5
    ./markov_generator.py --corpus-dir ./corpus/ --output output.txt

Author: VANTAGE Team
"""

# Standard library imports
import os
import sys
import logging
import argparse
import hashlib
from typing import List, Dict, Optional
from pathlib import Path

# Third-party imports (with robust error handling)
try:
    import markovify
    from tqdm import tqdm
    from unidecode import unidecode
except ImportError:
    print("Error: Required libraries not found. Install with:")
    print("pip install markovify numpy tqdm unidecode")
    sys.exit(1)

# Configure logging
LOG_DIR = Path(os.path.expanduser("~/logs"))
LOG_DIR.mkdir(parents=True, exist_ok=True)

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler(LOG_DIR / "markov_generator.log")
    ]
)
logger = logging.getLogger("vantage_markov")


class SecureTextLoader:
    """Securely loads and validates text files for processing."""

    def __init__(self, max_file_size: int = 10 * 1024 * 1024):
        """Initialize with maximum file size limit."""
        self.max_file_size = max_file_size

    def validate_file(self, file_path: str) -> bool:
        """Validate file exists, is readable, and isn't too large."""
        path = Path(file_path)

        if not path.exists():
            logger.error(f"File not found: {file_path}")
            return False

        if not path.is_file():
            logger.error(f"Not a file: {file_path}")
            return False

        if not os.access(file_path, os.R_OK):
            logger.error(f"File not readable: {file_path}")
            return False

        if path.stat().st_size > self.max_file_size:
            logger.error(f"File too large: {file_path} ({path.stat().st_size} bytes)")
            return False

        return True

    def load_file(self, file_path: str) -> Optional[str]:
        """Securely load a file with validation."""
        if not self.validate_file(file_path):
            return None

        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                text = f.read()

            # Calculate and log file hash for security auditing
            file_hash = hashlib.sha256(text.encode('utf-8')).hexdigest()
            logger.debug(f"Loaded file {file_path} (SHA256: {file_hash[:16]}...)")

            return text
        except Exception as e:
            logger.error(f"Error reading file {file_path}: {str(e)}")
            return None

    def load_directory(self, dir_path: str, ext: str = '.txt') -> Dict[str, str]:
        """Load all text files from a directory."""
        path = Path(dir_path)

        if not path.exists() or not path.is_dir():
            logger.error(f"Directory not found or not a directory: {dir_path}")
            return {}

        result = {}
        for file_path in tqdm(list(path.glob(f"*{ext}")), desc="Loading files"):
            content = self.load_file(str(file_path))
            if content:
                result[file_path.name] = content

        logger.info(f"Loaded {len(result)} files from {dir_path}")
        return result


class MarkovTextGenerator:
    """Generates text using markov chain models."""

    def __init__(
        self,
        state_size: int = 2,
        retention_ratio: float = 1.0,
        reject_reg: Optional[str] = None
    ):
        """Initialize the generator with given parameters."""
        self.state_size = state_size
        self.retention_ratio = retention_ratio
        self.reject_reg = reject_reg
        self.models = []
        self.loader = SecureTextLoader()

    def add_text(self, text: str, weight: float = 1.0) -> bool:
        """Add text to create a new model and combine it with existing ones."""
        if not text or len(text.strip()) == 0:
            logger.warning("Empty text provided, skipping")
            return False

        try:
            # Preprocess text to improve model quality
            processed_text = self._preprocess_text(text)

            # Build the model
            logger.info(f"Building Markov model (state size={self.state_size})")
            with tqdm(total=100, desc="Building model") as pbar:
                model = markovify.Text(
                    processed_text,
                    state_size=self.state_size,
                    retain_original=True,
                    well_formed=True
                )
                pbar.update(100)

            # Add the model to our collection with appropriate weight
            self.models.append((model, weight))
            logger.info(f"Added model with weight {weight} (total models: {len(self.models)})")
            return True

        except Exception as e:
            logger.error(f"Error building Markov model: {str(e)}")
            return False

    def add_file(self, file_path: str, weight: float = 1.0) -> bool:
        """Add a text file to the model."""
        text = self.loader.load_file(file_path)
        if text:
            return self.add_text(text, weight)
        return False

    def add_directory(self, dir_path: str, ext: str = '.txt') -> int:
        """Add all text files from a directory to the model."""
        files = self.loader.load_directory(dir_path, ext)
        success_count = 0

        # Calculate weights based on file sizes for better balance
        total_size = sum(len(content) for content in files.values())

        for name, content in files.items():
            # Weight by relative file size with a minimum weight
            weight = max(0.1, len(content) / total_size)
            if self.add_text(content, weight):
                success_count += 1

        return success_count

    def generate_sentence(self, max_chars: int = 280) -> Optional[str]:
        """Generate a single sentence from the model."""
        if not self.models:
            logger.error("No models available for text generation")
            return None

        try:
            # Combine models if we have multiple
            if len(self.models) == 1:
                model = self.models[0][0]
            else:
                model_weights = [weight for _, weight in self.models]
                models = [model for model, _ in self.models]
                model = markovify.combine(models, model_weights)

            # Try stricter generation first, then progressively relax constraints
            # for small corpora where overlap checks may reject every candidate.
            generation_attempts = (
                lambda: model.make_short_sentence(
                    max_chars=max_chars,
                    tries=100,
                    max_overlap_ratio=0.3,
                ),
                lambda: model.make_short_sentence(
                    max_chars=max_chars,
                    tries=200,
                    max_overlap_ratio=0.7,
                    test_output=False,
                ),
                lambda: model.make_sentence(
                    tries=200,
                    max_overlap_ratio=0.7,
                    test_output=False,
                ),
            )

            for attempt in generation_attempts:
                sentence = attempt()
                if sentence:
                    sentence = sentence.strip()
                    if sentence and len(sentence) <= max_chars:
                        return sentence

            return None

        except Exception as e:
            logger.error(f"Error generating text: {str(e)}")
            return None

    def generate_text(self, count: int = 5, max_chars: int = 280) -> List[str]:
        """Generate multiple sentences."""
        result = []

        with tqdm(total=count, desc="Generating text") as pbar:
            for _ in range(count):
                sentence = self.generate_sentence(max_chars)
                if sentence:
                    result.append(sentence)
                pbar.update(1)

        logger.info(f"Generated {len(result)} sentences")
        return result

    def _preprocess_text(self, text: str) -> str:
        """Preprocess text to improve model quality."""
        # Convert to ASCII to avoid unicode issues
        text = unidecode(text)

        # Remove excessive whitespace
        text = ' '.join(text.split())

        # Ensure text ends with terminal punctuation
        if text and text[-1] not in '.!?':
            text += '.'

        return text


def main():
    """Main function to handle command line arguments."""
    parser = argparse.ArgumentParser(description="VANTAGE Markov Text Generator")

    # Input sources
    input_group = parser.add_mutually_exclusive_group(required=True)
    input_group.add_argument('-i', '--input', help="Input text file path")
    input_group.add_argument('-d', '--corpus-dir', help="Directory containing text files")
    input_group.add_argument('-t', '--text', help="Direct text input")
    input_group.add_argument('-s', '--stdin', action='store_true', help="Read from standard input")

    # Output options
    parser.add_argument('-o', '--output', help="Output file path (default: stdout)")
    parser.add_argument('-c', '--count', type=int, default=5, help="Number of sentences to generate")
    parser.add_argument('-l', '--max-length', type=int, default=280, help="Maximum sentence length")

    # Model options
    parser.add_argument('-z', '--state-size', type=int, default=2, help="Markov chain state size")
    parser.add_argument('-r', '--retention', type=float, default=1.0, help="Model retention ratio")
    parser.add_argument('-e', '--extension', default='.txt', help="File extension when using corpus directory")

    # Security/debug options
    parser.add_argument('-v', '--verbose', action='store_true', help="Enable verbose logging")
    parser.add_argument('--max-file-size', type=int, default=10 * 1024 * 1024, help="Maximum file size in bytes")

    args = parser.parse_args()

    # Configure logging level
    if args.verbose:
        logger.setLevel(logging.DEBUG)

    # Create generator with specified parameters
    generator = MarkovTextGenerator(
        state_size=args.state_size,
        retention_ratio=args.retention
    )
    generator.loader.max_file_size = args.max_file_size

    # Add text from the specified source
    if args.input:
        if not generator.add_file(args.input):
            return 1
    elif args.corpus_dir:
        count = generator.add_directory(args.corpus_dir, args.extension)
        if count == 0:
            logger.error(f"No valid files found in {args.corpus_dir}")
            return 1
    elif args.text:
        if not generator.add_text(args.text):
            return 1
    elif args.stdin:
        logger.info("Reading from standard input...")
        text = sys.stdin.read()
        if not generator.add_text(text):
            return 1

    # Generate text
    sentences = generator.generate_text(args.count, args.max_length)

    if not sentences:
        logger.error("Failed to generate any sentences from the provided input")
        return 1

    # Output the generated text
    output_text = '\n'.join(sentences)
    if args.output:
        try:
            with open(args.output, 'w', encoding='utf-8') as f:
                f.write(output_text)
            logger.info(f"Output written to {args.output}")
        except Exception as e:
            logger.error(f"Error writing to {args.output}: {str(e)}")
            print(output_text)
    else:
        print(output_text)

    return 0


if __name__ == "__main__":
    sys.exit(main())
