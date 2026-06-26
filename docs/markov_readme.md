# VANTAGE Markov Text Generator

A secure, high-performance Markov chain text generator integrated with the VANTAGE framework. This module uses advanced text analysis and probabilistic models to generate natural-looking text based on input sources.

## Features

- **Intelligent Generation**: Advanced state-based Markov chain text generation
- **Corpus Management**: Add, list, and analyze corpus files
- **Secure Processing**: Validation of inputs and secure file handling
- **Terminal Integration**: Seamless integration with VANTAGE shell environment
- **Command Suggestions**: Optional integration with command prediction system

## Installation

The Markov generator is included with VANTAGE. To ensure all dependencies are installed:

```bash
# Activate the VANTAGE Python environment
source "$HOME/venv/bin/activate"

# Install dependencies
pip install markovify numpy tqdm unidecode
```

Enable the module in VANTAGE:

```bash
# Add to your modules configuration
echo "vantage_markov" >> ~/.bash_modules
```

## Usage

### Basic Text Generation

Generate text from an input file:

```bash
vantage_markov generate -i input.txt -o output.txt -s 3 -c 10
```

Where:
- `-i, --input`: Input file path (required)
- `-o, --output`: Output file path (optional)
- `-s, --state-size`: Markov chain state size (default: 2)
- `-c, --count`: Number of sentences to generate (default: 5)
- `-l, --max-length`: Maximum sentence length (default: 280)

### Corpus Management

Add a file to the corpus:

```bash
vantage_markov corpus ~/Documents/sample.txt
```

List all corpus files:

```bash
vantage_markov list
```

View corpus statistics:

```bash
vantage_markov corpus-stats
```

Clean cached data:

```bash
vantage_markov clean
```

## Advanced Configuration

The Markov generator can be customized through its configuration file. The default configuration is created on first run, but you can modify it to adjust behavior:

```json
{
    "state_size": 2,
    "default_count": 5,
    "max_length": 280,
    "retention_ratio": 1.0,
    "extensions": [".txt", ".md", ".rst"],
    "security": {
        "max_file_size": 10485760,
        "validate_input": true,
        "log_level": "info"
    }
}
```

## Technical Details

### Algorithm

The generator uses a state-based Markov chain that:

1. Analyzes input text and builds probabilistic state transitions
2. Generates new sequences based on learned probability distributions
3. Applies validation rules to ensure output quality and security

### Security Features

- Input validation and sanitization
- File size limits and format validation
- ASCII conversion to prevent unicode-based issues
- Permissioned file operations

## Integration with VANTAGE

The Markov generator integrates with other VANTAGE features:

- **Command Prediction**: Can suggest commands based on Markov analysis of shell history
- **Module System**: Respects VANTAGE's module dependency management
- **Logging System**: Integrates with VANTAGE's logging infrastructure

## Troubleshooting

- **Empty Output**: Input may be too small or contain incompatible formatting
- **Slow Performance**: Reduce state size or input file size
- **Permissions Errors**: Check file permissions in ${HOME}/markov/
- **Missing Dependencies**: Ensure all Python packages are installed

For detailed logs, check:
```bash
cat ${HOME}/logs/markov_generator.log
```

## License

This component is part of the VANTAGE project and is subject to the same license terms.

## Authors

- VANTAGE Team