# Module: sentinel_context

## Purpose
This module provides the core context-aware intelligence features of SENTINEL. It is responsible for gathering and analyzing context from the user's environment and using it to provide intelligent suggestions.

## Dependencies
- Required: `logging`, `python_integration`
- Optional: none

## Functions Exported
- `sentinel_context_show`: Shows the current context.
- `sentinel_context_update`: Updates the context.

## Configuration
- none

## Security Notes
- This module gathers information about your environment, including the files you are working on and the commands you are running. This information is stored locally and is not sent to any external servers.

## Examples
```bash
# Show the current context
sentinel_context_show

# Update the context
sentinel_context_update
```
