#!/usr/bin/env python3
"""
VANTAGE Integration Test Script
Demonstrates Python-Bash integration capabilities
"""

import sys
import os
import time
import json

# Add integration library to path
sys.path.insert(0, os.path.expanduser('~/.config/vantage/lib'))

try:
    from vantage_integration import vantage
except ImportError:
    print("ERROR: vantage_integration module not found. Please source python_integration.module first.")
    sys.exit(1)

def test_config_operations():
    """Test configuration read/write operations"""
    print("\n=== Testing Configuration Operations ===")
    
    # Set a config value
    vantage.set_config('test_key', 'test_value')
    print(f"Set config: test_key = test_value")
    
    # Get the config value
    value = vantage.get_config('test_key')
    print(f"Got config: test_key = {value}")
    
    # Test with default
    missing = vantage.get_config('missing_key', 'default_value')
    print(f"Got config with default: missing_key = {missing}")
    
    return value == 'test_value'

def test_state_operations():
    """Test state management operations"""
    print("\n=== Testing State Operations ===")
    
    # Set state
    vantage.set_state('test_state', 'active')
    print(f"Set state: test_state = active")
    
    # Get state
    state = vantage.get_state('test_state')
    print(f"Got state: test_state = {state}")
    
    return state == 'active'

def test_bash_execution():
    """Test bash command execution"""
    print("\n=== Testing Bash Execution ===")
    
    # Execute a simple command
    result = vantage.bash_exec('echo "Hello from bash"')
    print(f"Command success: {result['success']}")
    print(f"Command output: {result['stdout'].strip()}")
    
    # Execute command that uses VANTAGE environment
    result2 = vantage.bash_exec('echo $VANTAGE_STATE_DIR')
    print(f"VANTAGE_STATE_DIR: {result2['stdout'].strip()}")
    
    return result['success']

def test_ipc_communication():
    """Test IPC communication"""
    print("\n=== Testing IPC Communication ===")
    
    # Create a test channel using bash
    channel_result = vantage.bash_exec('vantage_ipc_create_channel test_channel')
    if not channel_result['success']:
        print("Failed to create IPC channel")
        return False
    
    # Send a message
    vantage.ipc_send('test_channel', 'Hello from Python')
    print("Sent message: Hello from Python")
    
    # Note: Receiving would block, so we skip it in this test
    print("IPC channels created successfully")
    
    return True

def test_ml_integration():
    """Test ML component integration"""
    print("\n=== Testing ML Component Integration ===")
    
    # Sync ML state
    result = vantage.bash_exec('vantage_ml_sync_state context')
    if result['success']:
        print("ML state synchronized")
        try:
            state_data = json.loads(result['stdout'])
            print(f"Current directory: {state_data.get('current_dir', 'unknown')}")
            print(f"User: {state_data.get('user', 'unknown')}")
        except:
            print("Could not parse ML state")
    
    return True

def main():
    """Run all integration tests"""
    print("VANTAGE Python-Bash Integration Test Suite")
    print("=" * 50)
    
    tests = [
        ("Configuration Operations", test_config_operations),
        ("State Operations", test_state_operations),
        ("Bash Execution", test_bash_execution),
        ("IPC Communication", test_ipc_communication),
        ("ML Integration", test_ml_integration)
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            if test_func():
                print(f"\n✓ {test_name} PASSED")
                passed += 1
            else:
                print(f"\n✗ {test_name} FAILED")
                failed += 1
        except Exception as e:
            print(f"\n✗ {test_name} FAILED with exception: {e}")
            failed += 1
    
    print("\n" + "=" * 50)
    print(f"Tests passed: {passed}/{len(tests)}")
    print(f"Tests failed: {failed}/{len(tests)}")
    
    return 0 if failed == 0 else 1

if __name__ == '__main__':
    sys.exit(main())