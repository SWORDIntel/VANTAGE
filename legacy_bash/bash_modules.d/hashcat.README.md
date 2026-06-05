# Module: hashcat

## Purpose
This module provides integration with Hashcat, the world's fastest and most advanced password recovery utility.

## Dependencies
- Required: `logging`
- Optional: none

## Functions Exported
- `sentinel_hashcat_run`: A wrapper function for running Hashcat with pre-configured options.
- `sentinel_hashcat_benchmark`: Runs the Hashcat benchmark.

## Configuration
- `HASHCAT_BIN`: The path to the Hashcat binary.
- `HASHCAT_WORDLISTS_DIR`: The directory containing the wordlists to use with Hashcat.
- `HASHCAT_OUTPUT_DIR`: The directory where the cracked passwords will be stored.

## Security Notes
- Hashcat is a powerful tool that can be used for malicious purposes. Use it responsibly and only on hashes that you are authorized to crack.
- It is recommended to run Hashcat as a non-root user.

## Examples
```bash
# Run Hashcat against a hash file
sentinel_hashcat_run -m 0 -a 0 /path/to/hashes.txt /path/to/wordlist.txt

# Run the Hashcat benchmark
sentinel_hashcat_benchmark
```
