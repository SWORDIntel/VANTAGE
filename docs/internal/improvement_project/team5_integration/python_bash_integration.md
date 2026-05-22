# Python/Bash Integration for SENTINEL

## Overview

This document describes the unified Python/Bash integration system implemented for SENTINEL, providing seamless interoperability between bash modules and Python components.

## Architecture

### Core Components

1. **python_integration.module** - Main bash module providing integration infrastructure
2. **sentinel_integration.py** - Python library for accessing bash functionality
3. **Shared State Management** - Unified state storage accessible from both languages
4. **IPC Mechanism** - Inter-process communication using named pipes
5. **Configuration System** - JSON-based shared configuration
6. **Error Handling** - Consistent error reporting across languages

### Directory Structure

```
~/.config/sentinel/
├── state/          # Shared state files
├── config/         # Configuration files
├── ipc/            # IPC named pipes
├── lib/            # Python integration library
└── venv/           # Python virtual environment

~/.local/share/sentinel/
└── logs/           # Unified logging
```

## Features

### 1. Unified State Management

Both bash and Python can read/write shared state:

**Bash:**
```bash
# Set state
sentinel_state_set "key" "value"

# Get state
value=$(sentinel_state_get "key")

# Delete state
sentinel_state_delete "key"
```

**Python:**
```python
from sentinel_integration import sentinel

# Set state
sentinel.set_state('key', 'value')

# Get state
value = sentinel.get_state('key')
```

### 2. Shared Configuration

Configuration is stored in JSON format and accessible from both languages:

**Bash:**
```bash
# Set configuration
sentinel_config_set "ml.enabled" "true"

# Get configuration
enabled=$(sentinel_config_get "ml.enabled")
```

**Python:**
```python
# Set configuration
sentinel.set_config('ml.enabled', True)

# Get configuration
enabled = sentinel.get_config('ml.enabled', False)
```

### 3. Inter-Process Communication (IPC)

Bidirectional communication between bash and Python processes:

**Bash:**
```bash
# Create channel
sentinel_ipc_create_channel "mychannel"

# Send message
sentinel_ipc_send "mychannel" "Hello from bash"

# Receive message (with 5-second timeout)
response=$(sentinel_ipc_receive "mychannel" 5)
```

**Python:**
```python
# Send message
sentinel.ipc_send('mychannel', 'Hello from Python')

# Receive message
message = sentinel.ipc_receive('mychannel', timeout=5)
```

### 4. Cross-Language Execution

Execute bash commands from Python and Python scripts from bash:

**From Python:**
```python
# Execute bash command
result = sentinel.bash_exec('ls -la')
if result['success']:
    print(result['stdout'])
else:
    print(f"Error: {result['stderr']}")
```

**From Bash:**
```bash
# Execute Python script with error handling
sentinel_python_exec "/path/to/script.py" arg1 arg2
```

### 5. Python Module Management

Integrated Python module management with virtual environment:

```bash
# Install Python module
sentinel_python_module_install "numpy"

# List installed modules
sentinel_python_module_list
```

### 6. ML Component Integration

Automatic state synchronization for ML components:

```bash
# Sync ML component state
sentinel_ml_sync_state "autolearn"

# Full ML sync
ml-sync

# Check ML status
ml-status
```

## Usage Examples

### Example 1: Bash Module Using Python

```bash
#!/usr/bin/env bash
# my_module.module

# Check if python_integration is loaded
if ! declare -f sentinel_state_get &>/dev/null; then
    echo "Error: python_integration module required"
    return 1
fi

# Use Python for complex processing
process_data() {
    local input="$1"
    
    # Store input in shared state
    sentinel_state_set "process_input" "$input"
    
    # Execute Python processor
    result=$(sentinel_python_exec "${SENTINEL_CONFIG_DIR}/processor.py")
    
    # Get result from shared state
    sentinel_state_get "process_result"
}
```

### Example 2: Python Script Using Bash Integration

```python
#!/usr/bin/env python3
# processor.py

from sentinel_integration import sentinel

# Get input from bash
input_data = sentinel.get_state('process_input')

# Process data
result = process_complex_data(input_data)

# Execute bash command
file_list = sentinel.bash_exec('find . -name "*.txt"')

# Store result for bash
sentinel.set_state('process_result', result)
```

### Example 3: ML Component with State Sync

```python
#!/usr/bin/env python3
# ml_component.py

from sentinel_integration import sentinel
import json

class MLComponent:
    def __init__(self):
        # Load configuration
        self.enabled = sentinel.get_config('ml.enabled', False)
        self.model_path = sentinel.get_config('ml.model_path', '~/models')
        
    def train(self):
        # Get training data from bash history
        history = sentinel.bash_exec('history | tail -1000')
        
        # Train model...
        
        # Update state
        sentinel.set_state('ml_last_training', str(time.time()))
        sentinel.set_config('ml.model_version', '1.2.3')
```

## Error Handling

The integration provides consistent error handling across languages:

### Bash Error Handling

```bash
# Errors are logged and displayed
sentinel_python_exec "script.py" || {
    echo "Python execution failed"
    sentinel_log "error" "Failed to execute script.py"
}
```

### Python Error Handling

```python
try:
    result = sentinel.bash_exec('complex_command')
    if not result['success']:
        sentinel.logger.error(f"Command failed: {result['stderr']}")
except Exception as e:
    sentinel.logger.error(f"Execution error: {e}")
```

## Performance Considerations

1. **State Storage**: Uses file-based storage for persistence, consider Redis for high-frequency updates
2. **IPC**: Named pipes have limited buffer size, use for small messages only
3. **Process Creation**: Minimize subprocess calls in tight loops
4. **Virtual Environment**: First module load creates venv, subsequent loads are faster

## Security Notes

1. **State Files**: Stored in user's home directory with user-only permissions
2. **IPC Channels**: Named pipes are created with restricted permissions
3. **Command Execution**: Always validate input before executing commands
4. **Configuration**: Sensitive data should be encrypted before storage

## Module Development Guidelines

### Creating Integration-Aware Modules

1. Always check for python_integration module:
```bash
if ! declare -f sentinel_state_get &>/dev/null; then
    echo "Error: python_integration module required"
    return 1
fi
```

2. Use state management for data exchange:
```bash
# Instead of temp files
sentinel_state_set "module_data" "$data"

# Instead of environment variables
value=$(sentinel_state_get "module_data")
```

3. Leverage Python for complex operations:
```bash
# Use Python for JSON processing
result=$(sentinel_python_exec -c "
import json
data = json.loads('$json_string')
print(data['key'])
")
```

### Python Component Guidelines

1. Always import sentinel_integration:
```python
from sentinel_integration import sentinel
```

2. Use logging for debugging:
```python
sentinel.logger.info("Processing started")
sentinel.logger.error("An error occurred")
```

3. Handle missing bash commands gracefully:
```python
result = sentinel.bash_exec('special_command')
if result['returncode'] == 127:  # Command not found
    sentinel.logger.warning("Command not available")
```

## Troubleshooting

### Common Issues

1. **Module not loaded error**
   - Solution: Source python_integration.module first
   ```bash
   source bash_modules.d/python_integration.module
   ```

2. **IPC timeout**
   - Check if both processes are running
   - Verify channel names match
   - Increase timeout value

3. **State not persisting**
   - Check directory permissions
   - Verify SENTINEL_STATE_DIR is set correctly

4. **Python module import errors**
   - Ensure virtual environment is activated
   - Install missing dependencies

### Debug Mode

Enable debug logging:
```bash
export SENTINEL_DEBUG=1
```

Check logs:
```bash
tail -f ~/.local/share/sentinel/logs/sentinel.log
tail -f ~/.local/share/sentinel/logs/python_integration.log
```

## Future Enhancements

1. **Redis Backend**: Optional Redis support for high-performance state management
2. **Message Queue**: RabbitMQ/ZeroMQ for complex IPC scenarios
3. **Async Support**: Asynchronous Python operations
4. **Remote Execution**: Distributed bash/Python execution
5. **State Encryption**: Built-in encryption for sensitive state data

## Conclusion

The Python/Bash integration system provides a robust foundation for building sophisticated SENTINEL modules that leverage the strengths of both languages. By providing unified state management, configuration, IPC, and error handling, developers can focus on functionality rather than integration complexity.