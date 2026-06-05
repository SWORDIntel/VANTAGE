# File: bash_modules

## Purpose
This file is the main entry point for the SENTINEL module system. It is responsible for loading all the other modules.

## Dependencies
- none

## Functions Exported
- `sentinel_load_module`: Loads a single module.
- `sentinel_load_modules`: Loads all modules listed in the `.bash_modules` file.

## Configuration
- none

## Security Notes
- This file is a critical part of the SENTINEL system. It should not be modified unless you know what you are doing.

## Examples
```bash
# Load a single module
sentinel_load_module logging

# Load all modules
sentinel_load_modules
```
