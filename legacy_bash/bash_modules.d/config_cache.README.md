# Module: config_cache

## Purpose
This module provides a system for caching configuration files to improve shell startup performance.

## Dependencies
- Required: `logging`
- Optional: none

## Functions Exported
- `vantage_config_cache_clear`: Clears the configuration cache.
- `vantage_config_cache_status`: Shows the status of the configuration cache.

## Configuration
- `VANTAGE_CONFIG_CACHE_ENABLED`: A flag to enable or disable the configuration cache.
- `VANTAGE_CONFIG_FORCE_REFRESH`: A flag to force a refresh of the configuration cache.
- `VANTAGE_CONFIG_CACHE_RETENTION_DAYS`: The number of days to keep cached configuration files.
- `VANTAGE_CONFIG_VERIFY_HASH`: A flag to enable or disable hash verification of cached files.

## Security Notes
- This module caches configuration files, which may contain sensitive information. The cache directory is protected with restrictive permissions, but it is still recommended to be aware of what is being cached.

## Examples
```bash
# Enable the configuration cache
export VANTAGE_CONFIG_CACHE_ENABLED=1

# Clear the configuration cache
vantage_config_cache_clear
```
