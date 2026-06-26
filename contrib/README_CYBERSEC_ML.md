# VANTAGE Cybersecurity ML Analyzer

Advanced machine learning module for cybersecurity analysis in the VANTAGE framework.

## Overview

The Cybersecurity ML Analyzer is an advanced security scanning tool that uses machine learning to detect potential security vulnerabilities in codebases. It combines traditional pattern matching with sophisticated ML techniques and LLM-powered analysis to provide comprehensive security insights.

## Features

- **Vulnerability Detection**: Identifies security issues using ML models and pattern matching
- **Multiple Languages**: Supports Python, JavaScript, PHP, Java, C/C++, and more
- **LLM-powered Analysis**: Uses local large language models for advanced code review
- **Vulnerability Database**: Maintains up-to-date security vulnerability information
- **Synthetic Data Generation**: Creates training examples for ML model improvement
- **Security Reporting**: Detailed reports with severity ratings and remediation advice
- **GitHub Integration**: Analyzes GitHub repositories for security concerns

## Installation

### Prerequisites

- Python 3.6+
- pip package manager

### Dependencies

The module requires several Python packages:

- **Core dependencies**: requests, numpy, tqdm, joblib, scikit-learn, scipy
- **Advanced ML** (optional): tensorflow
- **LLM capabilities** (optional): llama-cpp-python
- **TUI interface** (optional): npyscreen

### Installation Command

Install dependencies through the VANTAGE framework:

```bash
cyberinstall
# or
vantage_cybersec_install_deps
```

## Usage

### Quick Security Scan

For a quick security check of the current directory:

```bash
securitycheck
```

### Comprehensive Code Scanning

For more thorough analysis with additional options:

```bash
# Basic syntax
cyberscan <directory> [options]

# Examples
cyberscan ~/myproject --recursive
cyberscan /path/to/code --include=py,js,php --exclude=node_modules,vendor
cyberscan . --format json --output report.json
```

#### Scan Options

- `--recursive`: Scan subdirectories (default: false)
- `--include=<exts>`: Comma-separated list of file extensions to scan
- `--exclude=<dirs>`: Comma-separated list of directories to exclude
- `--format=<fmt>`: Output format (json, text, html)
- `--output=<file>`: Save results to specified file
- `--verbose`: Show detailed logging

### Updating Vulnerability Database

Keep the vulnerability database up to date:

```bash
cyberupdate
# or
cyberupdate --force # Skip time-based checks
```

### Training and Data Generation

Train machine learning models:

```bash
# Train models with existing data
cybertrain

# Generate synthetic training data
cyberdata --samples 200
```

### GitHub Repository Analysis

Analyze GitHub repositories for security issues:

```bash
cybergithub <username>
```

## Understanding Scan Results

Scan results include:

1. **Findings**: List of potential security issues found
2. **Severity**: Rating from 1 (low) to 5 (critical)
3. **Line Numbers**: Location of each issue in code
4. **Description**: Explanation of the security concern
5. **Mitigation**: Suggested fixes for each issue
6. **Confidence**: How confident the system is in each finding

### Example Output

```
Security Scan Results for /home/user/myproject
Files scanned: 143
Files with issues: 12
Total issues found: 27

Issues by severity:
  Severity 5: 3 issues
  Severity 4: 8 issues
  Severity 3: 12 issues
  Severity 2: 4 issues

Top issues:
1. SQL Injection: Unsanitized user input used in SQL query (Severity: 5)
   File: app/controllers/user.py:127
   Code: cursor.execute("SELECT * FROM users WHERE username = '" + username + "'")
   Mitigation: Use parameterized queries with placeholders

2. Command Injection: Shell command with user input (Severity: 5)
   File: scripts/process.js:45
   Code: exec('convert ' + userInput + ' output.png')
   Mitigation: Use proper argument separation and sanitization
...
```

## Technical Details

### Scanning Process

The scanning process involves multiple stages:

1. **File Collection**: Identifies relevant files for scanning
2. **Pattern Matching**: Checks for known vulnerability patterns
3. **ML Analysis**: Applies machine learning models to detect issues
4. **LLM Review**: Uses LLM for advanced contextual analysis
5. **Result Aggregation**: Combines findings and removes duplicates
6. **Severity Rating**: Assigns CVSS-compatible severity scores
7. **Reporting**: Generates comprehensive security reports

### Machine Learning Techniques

The tool uses several ML techniques:

- **TF-IDF Vectorization**: Converts code into numerical features
- **Random Forest Classification**: Classifies code as vulnerable or secure
- **Isolation Forest**: Detects anomalous code patterns
- **DBSCAN Clustering**: Groups similar findings together
- **Neural Networks** (if TensorFlow available): Deeper vulnerability analysis

### LLM Integration

When available, large language models provide:

- Advanced code analysis with contextual understanding
- Generation of vulnerable/secure code pairs for training
- Creation of detailed vulnerability signatures from CVE descriptions
- Summary of complex vulnerability patterns

## Configuration

The module can be configured by editing `~/.vantage/cybersec/config.json`:

```json
{
  "detection_threshold": 0.75,
  "analyze_libraries": true,
  "max_file_size": 10485760,
  "ignored_directories": [".git", "node_modules", "__pycache__", "venv"],
  "update_frequency": 604800,
  "api_endpoints": {
    "nvd": "https://services.nvd.nist.gov/rest/json/cves/2.0",
    "github_advisory": "https://api.github.com/advisories"
  },
  "llm_settings": {
    "temperature": 0.1,
    "max_tokens": 1024,
    "top_p": 0.9,
    "top_k": 40,
    "context_size": 4096
  }
}
```

## Best Practices

1. **Regular Updates**: Keep the vulnerability database current with `cyberupdate`
2. **Full Scans**: Run comprehensive scans before deploying code
3. **Review Findings**: Always manually verify critical findings
4. **Train Models**: Periodically retrain models with `cybertrain`
5. **Add Data**: Generate additional training data with `cyberdata`
6. **Targeted Scanning**: Use `--include` and `--exclude` for focused scans
7. **Integration**: Include security scans in your development workflow

## License

This module is part of the VANTAGE framework and is subject to the same license terms.

## Contributing

Contributions are welcome! Ways to contribute:

1. Report bugs and issues
2. Suggest new vulnerability patterns
3. Contribute training data
4. Improve ML models and algorithms
5. Add support for additional languages
6. Enhance documentation

For more details, see the main VANTAGE project documentation. 