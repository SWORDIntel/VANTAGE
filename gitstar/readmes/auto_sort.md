# VANTAGE Auto-Sort Scripts

Two Python scripts for automatically organizing files in the VANTAGE repository's gitstar/readmes structure.

## 1. `auto_sort.py`

Scans files in the UNSORTED directory and automatically sorts them to appropriate category directories based on keyword matching.

### Features:
- Extensive keyword dictionary for accurately categorizing content
- Multiple encoding support for reading files
- Configurable minimum match threshold
- Dry-run option for testing before moving files
- Detailed reporting of results

### Usage:

```bash
# Run with default settings
./auto_sort.py

# Dry run (no actual file moving)
./auto_sort.py --dry-run

# Adjust minimum score threshold
./auto_sort.py --min-score 3

# Specify custom directories
./auto_sort.py --unsorted-dir custom/path/to/unsorted --dest-base custom/path/to/categories
```

## 2. `auto_sort_results.py`

Updates the `progress.md` file with newly categorized files, maintaining a documented record of organization.

### Features:
- Detects files already in progress.md to avoid duplicates
- Automatically generates basic tags based on categories
- Updates timestamp in the progress.md file
- Sorts entries alphabetically

### Usage:

```bash
# Run with default settings
./auto_sort_results.py

# Specify custom progress.md file
./auto_sort_results.py --progress-file custom/path/to/progress.md
```

## Workflow:

1. Run `auto_sort.py` to classify and move files
2. Review the results
3. Run `auto_sort_results.py` to update progress.md with the new categorizations

This automation helps maintain an organized repository structure with minimal manual intervention. 