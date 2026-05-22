# SENTINEL Module Health Check System

## Overview

The Health Check module provides comprehensive monitoring, diagnostics, and automatic recovery capabilities for the SENTINEL module system. It continuously monitors module health, detects issues, and can automatically attempt recovery procedures to maintain system stability.

## Features

### 1. **Comprehensive Health Monitoring**
- Real-time health status tracking for all loaded modules
- Configurable health check intervals
- Memory usage monitoring
- Dependency health tracking
- Performance degradation detection

### 2. **Automatic Recovery Mechanisms**
- Soft recovery for minor issues (cache clearing, state reset)
- Module reloading for errors
- Quarantine system for critically failed modules
- Configurable retry limits

### 3. **Health Status Levels**
- `OK` (0): Module functioning normally
- `WARNING` (1): Minor issues detected, module still functional
- `ERROR` (2): Significant issues, recovery attempted
- `CRITICAL` (3): Module failure, quarantine recommended
- `UNKNOWN` (4): Module not loaded or status unavailable

### 4. **Logging Integration**
- All health events logged via SENTINEL logging system
- Detailed issue tracking
- Recovery attempt history
- Performance metrics

## Installation

1. The health check module is automatically installed with SENTINEL
2. To manually enable:
   ```bash
   module_enable health_check
   ```

## Configuration

### Environment Variables

```bash
# Enable/disable health monitoring (default: 1)
export SENTINEL_HEALTH_CHECK_ENABLED=1

# Health check interval in seconds (default: 300)
export SENTINEL_HEALTH_CHECK_INTERVAL=300

# Enable automatic recovery (default: 1)
export SENTINEL_HEALTH_CHECK_AUTO_RECOVERY=1

# Maximum recovery attempts (default: 3)
export SENTINEL_HEALTH_CHECK_MAX_RETRIES=3

# Memory threshold in KB (default: 100000)
export SENTINEL_HEALTH_MEMORY_THRESHOLD=100000

# Module load time threshold in ms (default: 5000)
export SENTINEL_HEALTH_LOAD_TIME_THRESHOLD=5000

# Run health checks in background (default: 1)
export SENTINEL_HEALTH_CHECK_BACKGROUND=1
```

## Usage

### Command Line Interface

```bash
# Check health status of all modules
health_check status

# Check specific module health
health_check check module_name

# Check all modules immediately
health_check check

# Enable periodic health monitoring
health_check enable

# Disable periodic health monitoring
health_check disable

# Manually trigger recovery for a module
health_check recover module_name

# Clear quarantine status for a module
health_check clear module_name
```

### Example Output

```
SENTINEL Module Health Status
============================

Module                         Status     Last Check           Failures
------                         ------     ----------           --------
autocomplete                   OK         23s ago              0
config_cache                   OK         23s ago              0
fuzzy_correction              WARNING     23s ago              1
logging                       OK         23s ago              0
sentinel_ml                   ERROR       23s ago              3

Recovery Attempts:
  sentinel_ml: 2 attempts
```

## Module-Specific Health Checks

### Core Modules

#### logging
- Checks log directory accessibility
- Verifies logging function availability

#### config_cache
- Validates cache directory
- Monitors cache size (100MB limit)

#### shell_security
- Verifies security functions
- Checks security violation log

### ML Modules

#### sentinel_ml
- Python availability check
- Model directory validation
- Cache directory check

#### sentinel_chat
- API configuration validation
- Function availability check

### Performance Modules

#### fuzzy_correction
- Memory usage monitoring
- Cache size limits

#### autocomplete
- Completion count limits
- Directory accessibility

## Recovery Strategies

### 1. Soft Recovery
Triggered for WARNING status:
- Clear module caches
- Reset internal state
- Reset failure counters

### 2. Module Reload
Triggered for ERROR status:
- Unload module functions
- Clear module state
- Reload module
- Verify recovery

### 3. Quarantine
Triggered for CRITICAL status or max retries:
- Disable module
- Create quarantine marker
- Notify user
- Require manual intervention

## Custom Health Checks

### Registering Custom Checks

```bash
# Define a health check function
my_module_health_check() {
    # Return 0 for healthy, non-zero for unhealthy
    [[ -f "/required/file" ]] || return 1
    return 0
}

# Register the health check
register_health_check "my_module" my_module_health_check
```

### Health Check Function Requirements

1. Must return appropriate exit codes:
   - 0: Healthy
   - 1: Unhealthy
   - 2+: Critical

2. Should complete quickly (< 1 second)

3. Should not modify system state

4. Error output will be captured as issue description

## Monitoring Best Practices

### 1. Regular Status Checks
```bash
# Add to your shell profile for startup check
health_check status
```

### 2. Automated Monitoring
```bash
# Enable background monitoring
health_check enable

# Check logs periodically
tail -f ~/.cache/sentinel/health/health_$(date +%Y%m%d).log
```

### 3. Recovery Procedures

For modules in quarantine:
1. Check the quarantine reason:
   ```bash
   cat ~/.cache/sentinel/health/quarantine_module_name
   ```

2. Fix underlying issues

3. Clear quarantine:
   ```bash
   health_check clear module_name
   ```

4. Re-enable module:
   ```bash
   module_enable module_name
   ```

## Troubleshooting

### Common Issues

#### High Memory Usage
- Check `SENTINEL_HEALTH_MEMORY_THRESHOLD`
- Clear module caches
- Restart shell session

#### Repeated Failures
- Check module dependencies
- Verify system requirements
- Review module logs

#### False Positives
- Adjust thresholds
- Customize health checks
- Increase check intervals

### Debug Mode

Enable detailed logging:
```bash
export SENTINEL_DEBUG_MODULES=1
export SENTINEL_LOG_LEVEL=0
```

## Performance Impact

The health check system is designed to be lightweight:
- Minimal memory overhead
- Rate-limited checks (1 minute minimum between checks)
- Background operation
- Cached status results

## Integration with Other Modules

### Logging Module
- All health events are logged
- Configurable log levels
- Persistent health history

### Performance Monitor
- Shares performance metrics
- Coordinated monitoring
- Unified reporting

## Future Enhancements

1. **Predictive Health Monitoring**
   - Trend analysis
   - Failure prediction
   - Proactive recovery

2. **Health Metrics Dashboard**
   - Real-time visualization
   - Historical trends
   - Performance graphs

3. **Remote Monitoring**
   - Network health checks
   - Distributed monitoring
   - Alert notifications

4. **Advanced Recovery**
   - Dependency-aware recovery
   - Cascading recovery
   - State preservation

## API Reference

### Functions

#### `check_module_health(module_name, force)`
Check health of a specific module.
- `module_name`: Module to check
- `force`: Skip rate limiting (0/1)
- Returns: Health status code

#### `check_all_modules_health()`
Check health of all loaded modules.
- Returns: Count of unhealthy modules

#### `register_health_check(module_name, check_function)`
Register a custom health check.
- `module_name`: Module name
- `check_function`: Function name to call

### Variables

- `SENTINEL_MODULE_HEALTH_STATUS`: Associative array of module health states
- `SENTINEL_MODULE_FAILURE_COUNT`: Failure counts per module
- `SENTINEL_MODULE_RECOVERY_ATTEMPTS`: Recovery attempt counts

## Changelog

### Version 1.0.0
- Initial implementation
- Core health monitoring
- Automatic recovery
- Module-specific checks
- CLI interface