#!/usr/bin/env python3
"""
Keyword extractor for VANTAGE repository
Analyzes directories and extracts keywords suitable for auto_sort.py categories
"""

import os
import re
import argparse
from pathlib import Path
from collections import Counter, defaultdict
import string
import json
from concurrent.futures import ProcessPoolExecutor, as_completed

# Define common stop words to exclude
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

# Technical words that are common across many categories (to be excluded)
COMMON_TECH_WORDS = {
    'github', 'com', 'http', 'https', 'www', 'repository', 'repo', 'readme',
    'license', 'file', 'files', 'directory', 'folder', 'version', 'release',
    'download', 'install', 'build', 'make', 'run', 'code', 'src', 'example',
    'examples', 'documentation', 'docs', 'project', 'support', 'issue', 'issues',
    'pull', 'request', 'requests', 'contribute', 'contributing', 'contributor',
    'contributors', 'test', 'tests', 'testing', 'development', 'developer',
    'developers', 'author', 'authors', 'license', 'copyright'
}

def read_file_content(file_path):
    """Read file content safely, handling encoding issues."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except UnicodeDecodeError:
        try:
            with open(file_path, 'r', encoding='latin-1') as f:
                return f.read()
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return ""

def clean_text(text):
    """Clean text by removing punctuation, special characters, and numbers."""
    # Replace URLs with a space
    text = re.sub(r'http[s]?://\S+', ' ', text)
    
    # Replace markdown links with a space
    text = re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', text)
    
    # Replace backslashes with a space
    text = text.replace('\\', ' ')
    
    # Replace punctuation with a space
    for char in string.punctuation:
        text = text.replace(char, ' ')
    
    # Remove digits
    text = re.sub(r'\d+', ' ', text)
    
    # Convert to lowercase
    text = text.lower()
    
    # Replace multiple spaces with a single space
    text = re.sub(r'\s+', ' ', text)
    
    return text

def extract_words(file_path, min_length=3):
    """Extract meaningful words from a file."""
    content = read_file_content(file_path)
    if not content:
        return Counter()
    
    cleaned_text = clean_text(content)
    words = cleaned_text.split()
    
    # Filter out stop words, common tech words, and words that are too short
    meaningful_words = [
        word for word in words 
        if word not in STOP_WORDS 
        and word not in COMMON_TECH_WORDS
        and len(word) >= min_length
    ]
    
    return Counter(meaningful_words)

def process_directory(directory, min_length=3, exclude_files=None, worker_count=None):
    """Process all markdown files in a directory."""
    if exclude_files is None:
        exclude_files = []
    
    # Get all markdown files
    path = Path(directory)
    all_files = list(path.glob('*.md'))
    
    # Filter out excluded files
    files_to_process = [f for f in all_files if f.name not in exclude_files]
    
    print(f"Processing {len(files_to_process)} files in {directory}")
    
    # Process files in parallel
    word_counter = Counter()
    
    if worker_count is None:
        worker_count = os.cpu_count()
    
    # Create batches for parallel processing
    batch_size = max(1, len(files_to_process) // worker_count)
    batches = [files_to_process[i:i + batch_size] for i in range(0, len(files_to_process), batch_size)]
    
    with ProcessPoolExecutor(max_workers=worker_count) as executor:
        futures = []
        for batch in batches:
            future = executor.submit(process_batch, batch, min_length)
            futures.append(future)
            
        for future in as_completed(futures):
            try:
                batch_counter = future.result()
                word_counter.update(batch_counter)
            except Exception as e:
                print(f"Error processing batch: {e}")
    
    return word_counter

def process_batch(files, min_length):
    """Process a batch of files in parallel."""
    batch_counter = Counter()
    for file_path in files:
        file_counter = extract_words(file_path, min_length)
        batch_counter.update(file_counter)
    return batch_counter

def analyze_categories(base_dir, categories=None, min_length=3, min_count=5, top_n=50, exclude_files=None, worker_count=None):
    """Analyze multiple category directories and extract distinctive keywords."""
    base_path = Path(base_dir)
    
    if categories is None:
        # Get all subdirectories
        categories = [d.name for d in base_path.iterdir() if d.is_dir() and d.name != 'UNSORTED']
    
    print(f"Analyzing categories: {', '.join(categories)}")
    
    # Process each category
    category_counters = {}
    all_words_counter = Counter()
    
    for category in categories:
        category_path = base_path / category
        if not category_path.is_dir():
            print(f"Warning: {category} is not a directory")
            continue
        
        counter = process_directory(
            category_path, 
            min_length=min_length, 
            exclude_files=exclude_files,
            worker_count=worker_count
        )
        
        # Filter words that appear less than min_count times
        filtered_counter = Counter({word: count for word, count in counter.items() if count >= min_count})
        
        category_counters[category] = filtered_counter
        all_words_counter.update(filtered_counter)
    
    # Find distinctive words for each category
    distinctive_keywords = {}
    
    for category, counter in category_counters.items():
        # Calculate distinctiveness score
        # Higher score = more distinctive to this category
        distinctive_words = []
        
        for word, count in counter.most_common():
            # Calculate what percentage of total occurrences are in this category
            total_occurrences = all_words_counter[word]
            distinctiveness = count / total_occurrences
            
            # Only include words that appear predominantly in this category
            if distinctiveness >= 0.5:  # At least 50% of occurrences in this category
                distinctive_words.append((word, count, distinctiveness))
        
        # Sort by count and take top N
        distinctive_words.sort(key=lambda x: (x[2], x[1]), reverse=True)
        distinctive_keywords[category] = [
            {"word": w, "count": c, "distinctiveness": d}
            for w, c, d in distinctive_words[:top_n]
        ]
    
    return distinctive_keywords

def format_for_auto_sort(keywords):
    """Format keywords for use in auto_sort.py"""
    formatted = {}
    
    for category, words in keywords.items():
        formatted[category] = [item["word"] for item in words]
    
    return formatted

def main():
    """Main function to extract keywords."""
    parser = argparse.ArgumentParser(description='Extract distinctive keywords from category directories.')
    parser.add_argument('--base-dir', default='gitstar/readmes', help='Base directory containing category subdirectories')
    parser.add_argument('--categories', help='Comma-separated list of categories to analyze (default: all directories)')
    parser.add_argument('--min-length', type=int, default=3, help='Minimum word length to consider')
    parser.add_argument('--min-count', type=int, default=5, help='Minimum count to include a word')
    parser.add_argument('--top-n', type=int, default=50, help='Number of top words per category to include')
    parser.add_argument('--exclude', default='progress.md', help='Comma-separated list of files to exclude')
    parser.add_argument('--workers', type=int, default=os.cpu_count(), help='Number of worker processes')
    parser.add_argument('--output', default='keywords.json', help='Output file for keywords')
    parser.add_argument('--format', choices=['json', 'auto_sort'], default='json', 
                        help='Output format: detailed JSON or auto_sort.py compatible')
    
    args = parser.parse_args()
    
    # Parse categories if provided
    categories = None
    if args.categories:
        categories = [c.strip() for c in args.categories.split(',')]
    
    # Parse excluded files
    exclude_files = [f.strip() for f in args.exclude.split(',')]
    
    # Analyze categories
    keywords = analyze_categories(
        args.base_dir,
        categories=categories,
        min_length=args.min_length,
        min_count=args.min_count,
        top_n=args.top_n,
        exclude_files=exclude_files,
        worker_count=args.workers
    )
    
    # Format keywords if needed
    if args.format == 'auto_sort':
        keywords = format_for_auto_sort(keywords)
    
    # Save keywords to file
    with open(args.output, 'w', encoding='utf-8') as f:
        json.dump(keywords, f, indent=2)
    
    print(f"Keywords saved to {args.output}")
    
    # Print sample of keywords for each category
    print("\nSample keywords for each category:")
    for category, words in keywords.items():
        if args.format == 'auto_sort':
            sample = words[:10]
        else:
            sample = [item["word"] for item in words[:10]]
        
        print(f"{category}: {', '.join(sample)}")

if __name__ == "__main__":
    main() 