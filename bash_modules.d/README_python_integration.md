# Python Integration Module for SENTINEL

## Quick Start

1. **Load the module:**
   ```bash
   source bash_modules.d/python_integration.module
   ```

2. **Test the integration:**
   ```bash
   ./test_python_integration.sh
   ```

3. **Use in your modules:**
   ```bash
   # Check if integration is loaded
   if ! declare -f sentinel_state_get &>/dev/null; then
       echo "Error: python_integration required"
       return 1
   fi
   
   # Use the features
   sentinel_state_set "mykey" "myvalue"
   result=$(sentinel_python_exec "myscript.py")
   ```

## Key Features

### State Management
- `sentinel_state_get/set/delete` - Manage shared state
- `sentinel_config_get/set` - Manage configuration

### Python Execution
- `sentinel_python_exec` - Execute Python with error handling
- `sentinel_python_module_install/list` - Manage Python dependencies

### IPC Communication
- `sentinel_ipc_create_channel` - Create communication channel
- `sentinel_ipc_send/receive` - Send/receive messages

### ML Integration
- `sentinel_ml_sync_state` - Synchronize ML component state
- Automatic state persistence

## Example Modules

1. **ml_state_sync.module** - Automatic ML state synchronization
2. **example_integrated.module** - Complete integration examples

## Python Side

From Python, use the sentinel_integration library:

```python
from sentinel_integration import sentinel

# Access bash functionality
result = sentinel.bash_exec('ls -la')
sentinel.set_state('key', 'value')
config = sentinel.get_config('setting', 'default')
```

## Files Created

- `~/.config/sentinel/state/` - State storage
- `~/.config/sentinel/config/` - Configuration files
- `~/.config/sentinel/ipc/` - IPC channels
- `~/.config/sentinel/lib/sentinel_integration.py` - Python library
- `~/.local/share/sentinel/logs/` - Log files

## Troubleshooting

1. **Module not loaded:** Source python_integration.module first
2. **Python errors:** Check logs in ~/.local/share/sentinel/logs/
3. **IPC timeout:** Increase timeout parameter or check channel names
4. **State not persisting:** Check directory permissions

## Advanced Usage

See `/opt/github/SENTINEL/docs/internal/improvement_project/team5_integration/python_bash_integration.md` for comprehensive documentation.