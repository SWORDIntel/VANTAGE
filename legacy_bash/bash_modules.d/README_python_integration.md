# Python Integration Module for VANTAGE

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
   if ! declare -f vantage_state_get &>/dev/null; then
       echo "Error: python_integration required"
       return 1
   fi
   
   # Use the features
   vantage_state_set "mykey" "myvalue"
   result=$(vantage_python_exec "myscript.py")
   ```

## Key Features

### State Management
- `vantage_state_get/set/delete` - Manage shared state
- `vantage_config_get/set` - Manage configuration

### Python Execution
- `vantage_python_exec` - Execute Python with error handling
- `vantage_python_module_install/list` - Manage Python dependencies

### IPC Communication
- `vantage_ipc_create_channel` - Create communication channel
- `vantage_ipc_send/receive` - Send/receive messages

### ML Integration
- `vantage_ml_sync_state` - Synchronize ML component state
- Automatic state persistence

## Example Modules

1. **ml_state_sync.module** - Automatic ML state synchronization
2. **example_integrated.module** - Complete integration examples

## Python Side

From Python, use the vantage_integration library:

```python
from vantage_integration import vantage

# Access bash functionality
result = vantage.bash_exec('ls -la')
vantage.set_state('key', 'value')
config = vantage.get_config('setting', 'default')
```

## Files Created

- `~/.config/vantage/state/` - State storage
- `~/.config/vantage/config/` - Configuration files
- `~/.config/vantage/ipc/` - IPC channels
- `~/.config/vantage/lib/vantage_integration.py` - Python library
- `~/.local/share/vantage/logs/` - Log files

## Troubleshooting

1. **Module not loaded:** Source python_integration.module first
2. **Python errors:** Check logs in ~/.local/share/vantage/logs/
3. **IPC timeout:** Increase timeout parameter or check channel names
4. **State not persisting:** Check directory permissions

## Advanced Usage

See `/opt/github/VANTAGE/docs/internal/improvement_project/team5_integration/python_bash_integration.md` for comprehensive documentation.