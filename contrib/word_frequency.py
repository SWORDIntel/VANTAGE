#!/usr/bin/env python3
"""
Word frequency analyzer for VANTAGE repository.
Lists the top N most frequent words across all files in a directory.
"""

import argparse
import os
import re
import sys
import string
from collections import Counter
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
from typing import Iterable, Iterator, List

STOP_WORDS = {
    'the', 'and', 'a', 'to', 'of', 'in', 'is', 'that', 'it', 'for',
    'with', 'as', 'this', 'on', 'be', 'are', 'by', 'an', 'was', 'can',
    'from', 'or', 'you', 'have', 'not', 'will', 'at', 'your', 'all', 'has',
    'we', 'been', 'if', 'they', 'their', 'but', 'when', 'what', 'which',
    'so', 'there', 'no', 'would', 'our', 'about', 'who', 'its', 'only',
    'also', 'them', 'than', 'then', 'some', 'my', 'other', 'do', 'more',
    'using', 'used', 'these', 'such', 'use', 'any', 'up', 'may', 'should',
    'could', 'how', 'into', 'one', 'out', 'like', 'just', 'each', 'after',
    'through', 'before', 'between', 'those', 'over', 'under', 'very', 'were',
    'had', 'he', 'she', 'his', 'her', 'i', 'me', 'am', 'us', 'him', 'hers',
    'we', 'they', 'them', 'our', 'their', 'theirs', 'being', 'been', 'did',
    'does', 'most'
}


def read_file_content(file_path: Path) -> str:
    """Read file content safely, handling encoding issues."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except UnicodeDecodeError:
        try:
            with open(file_path, 'r', encoding='latin-1') as f:
                return f.read()
        except Exception as exc:  # pragma: no cover - logging only
            print(f"Error reading {file_path}: {exc}")
            return ""


def clean_text(text: str, include_code: bool) -> str:
    """Clean text by removing punctuation, special characters, and numbers."""
    if not include_code:
        text = re.sub(r"```.*?```", " ", text, flags=re.DOTALL)

    text = re.sub(r'http[s]?://\S+', ' ', text)
    text = re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', text)
    text = text.replace('\\', ' ')

    for char in string.punctuation:
        text = text.replace(char, ' ')

    text = re.sub(r'\d+', ' ', text)
    text = text.lower()
    text = re.sub(r'\s+', ' ', text)

    return text


def extract_words(file_path: Path, include_code: bool) -> Counter:
    """Extract meaningful words from a file."""
    content = read_file_content(file_path)
    if not content:
        return Counter()

    cleaned_text = clean_text(content, include_code=include_code)
    words = cleaned_text.split()
    meaningful_words = [word for word in words if word not in STOP_WORDS and len(word) > 2]
    return Counter(meaningful_words)


def process_files_batch(files: Iterable[Path], include_code: bool) -> Counter:
    """Process a batch of files and return their word counts."""
    word_counter = Counter()
    for file_path in files:
        file_counter = extract_words(file_path, include_code=include_code)
        word_counter.update(file_counter)
    return word_counter


def chunked(iterable: List[Path], size: int) -> Iterator[List[Path]]:
    """Yield successive chunks from a list."""
    for idx in range(0, len(iterable), size):
        yield iterable[idx:idx + size]


def main() -> int:
    """Main function to analyze word frequency."""
    script_dir = Path(__file__).resolve().parent
    default_dir = Path(os.environ.get(
        "VANTAGE_WORD_FREQ_DIR",
        script_dir / "gitstar" / "readmes" / "UNSORTED",
    ))

    parser = argparse.ArgumentParser(description='Analyze word frequency in files.')
    parser.add_argument('--dir', default=None, help='Directory to analyze')
    parser.add_argument('--top', type=int, default=20, help='Number of top words to display')
    parser.add_argument('--workers', type=int, default=os.cpu_count(), help='Number of worker processes')
    parser.add_argument('--exclude', type=str, default='progress.md', help='Comma-separated list of files to exclude')
    parser.add_argument('--min-length', type=int, default=3, help='Minimum word length to consider')
    parser.add_argument('--include-code', action='store_true', help='Include words from code blocks')

    args = parser.parse_args()

    target_dir = Path(args.dir) if args.dir else default_dir
    if not target_dir.exists():
        print(f"Directory does not exist: {target_dir}")
        return 1

    all_files: List[Path] = list(target_dir.glob('*.md'))
    if not all_files:
        print(f"No markdown files found under {target_dir}")
        return 0

    exclude_files = {name.strip() for name in args.exclude.split(',') if name.strip()}
    files_to_process = [f for f in all_files if f.name not in exclude_files]

    if not files_to_process:
        print(f"All files were excluded under {target_dir}")
        return 0

    print(f"Found {len(files_to_process)} files to analyze")

    max_workers = max(1, args.workers or 1)
    batch_size = max(1, len(files_to_process) // max_workers)
    batches = list(chunked(files_to_process, batch_size))

    word_counter = Counter()
    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = [executor.submit(process_files_batch, batch, args.include_code) for batch in batches]
        for future in as_completed(futures):
            try:
                batch_counter = future.result()
                word_counter.update(batch_counter)
            except Exception as exc:  # pragma: no cover - logging only
                print(f"Error processing batch: {exc}")

    if args.min_length > 2:
        word_counter = Counter({
            word: count for word, count in word_counter.items()
            if len(word) >= args.min_length
        })

    if not word_counter:
        print("No words collected after filtering")
        return 0

    top_words = word_counter.most_common(args.top)
    print(f"\nTop {args.top} words in {target_dir}:")
    max_word_len = max(len(word) for word, _ in top_words)
    max_count_len = max(len(str(count)) for _, count in top_words)

    print("\n{:<{}} | {:<{}} | {}".format("Word", max_word_len, "Count", max_count_len, "Percentage"))
    print("-" * (max_word_len + max_count_len + 15))

    total_words = sum(word_counter.values())
    for word, count in top_words:
        percentage = (count / total_words) * 100
        print("{:<{}} | {:<{}} | {:.2f}%".format(word, max_word_len, count, max_count_len, percentage))

    print(f"\nTotal unique words: {len(word_counter)}")
    print(f"Total words analyzed: {total_words}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
