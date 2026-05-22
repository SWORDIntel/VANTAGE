# MCP (Model Context Protocol) Integration Example

## Overview

The Model Context Protocol (MCP) enables AI models to interact with SENTINEL's external tools through a standardized JSON-based protocol. This allows for seamless integration between AI assistants and system tools.

## Architecture

```
AI Model <-> MCP Client <-> MCP Socket <-> SENTINEL MCP Handler <-> External Tools
```

## Setup

1. Enable MCP in your SENTINEL configuration:
```bash
export SENTINEL_MCP_ENABLED=1
export SENTINEL_MCP_SOCKET="$HOME/.sentinel/tools/mcp.sock"
export SENTINEL_MCP_TIMEOUT=30
```

2. Load the external tools module:
```bash
source ~/.sentinel/modules/external_tools.module
```

## Protocol Specification

### Request Format

```json
{
  "id": "unique-request-id",
  "method": "tool.method",
  "params": {
    "param1": "value1",
    "param2": "value2"
  }
}
```

### Response Format

```json
{
  "id": "unique-request-id",
  "result": {
    "status": "success",
    "data": {}
  }
}
```

### Error Response

```json
{
  "id": "unique-request-id",
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

## Available Methods

### 1. tool.list
List all available tools.

Request:
```json
{
  "id": "123",
  "method": "tool.list",
  "params": {}
}
```

Response:
```json
{
  "id": "123",
  "result": {
    "git": {
      "path": "/usr/bin/git",
      "type": "vcs",
      "enabled": true
    },
    "docker": {
      "path": "/usr/bin/docker",
      "type": "container",
      "enabled": true
    }
  }
}
```

### 2. tool.execute
Execute a registered tool.

Request:
```json
{
  "id": "124",
  "method": "tool.execute",
  "params": {
    "tool": "git",
    "args": ["status", "--porcelain"]
  }
}
```

Response:
```json
{
  "id": "124",
  "result": {
    "status": 0,
    "stdout": "M README.md\nA new_file.txt",
    "stderr": ""
  }
}
```

### 3. tool.info
Get detailed information about a tool.

Request:
```json
{
  "id": "125",
  "method": "tool.info",
  "params": {
    "tool": "docker"
  }
}
```

Response:
```json
{
  "id": "125",
  "result": {
    "path": "/usr/bin/docker",
    "metadata": {
      "type": "container",
      "category": "infrastructure",
      "version": "24.0.0"
    },
    "registered": "2024-01-15T10:30:00Z",
    "enabled": true
  }
}
```

## Plugin-Specific Handlers

Plugins can register their own MCP handlers for specialized operations.

### Git Plugin Methods

#### git.status
Get repository status in JSON format.

Request:
```json
{
  "id": "201",
  "method": "git.status",
  "params": {}
}
```

#### git.commit
Create a commit with message.

Request:
```json
{
  "id": "202",
  "method": "git.commit",
  "params": {
    "message": "Fix: Resolve issue #123"
  }
}
```

### Docker Plugin Methods

#### docker.ps
List running containers.

Request:
```json
{
  "id": "301",
  "method": "docker.ps",
  "params": {
    "all": false
  }
}
```

#### docker.scan
Security scan an image.

Request:
```json
{
  "id": "302",
  "method": "docker.scan",
  "params": {
    "image": "nginx:latest",
    "type": "trivy"
  }
}
```

### Kubernetes Plugin Methods

#### k8s.get
Get Kubernetes resources.

Request:
```json
{
  "id": "401",
  "method": "k8s.get",
  "params": {
    "resource": "pods",
    "namespace": "default"
  }
}
```

## Client Implementation Example

### Python Client

```python
import json
import socket
import os

class MCPClient:
    def __init__(self, socket_path="~/.sentinel/tools/mcp.sock"):
        self.socket_path = os.path.expanduser(socket_path)
        self.request_id = 0
    
    def send_request(self, method, params=None):
        self.request_id += 1
        request = {
            "id": str(self.request_id),
            "method": method,
            "params": params or {}
        }
        
        # Send request
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.connect(self.socket_path)
            s.send(json.dumps(request).encode())
            
            # Wait for response
            response = s.recv(4096).decode()
            return json.loads(response)
    
    def list_tools(self):
        return self.send_request("tool.list")
    
    def execute_tool(self, tool, args):
        return self.send_request("tool.execute", {
            "tool": tool,
            "args": args
        })

# Usage example
client = MCPClient()

# List available tools
tools = client.list_tools()
print("Available tools:", tools)

# Execute git status
result = client.execute_tool("git", ["status"])
print("Git status:", result)
```

### Bash Client

```bash
#!/usr/bin/env bash
# MCP client implementation in bash

MCP_SOCKET="${MCP_SOCKET:-$HOME/.sentinel/tools/mcp.sock}"

mcp_request() {
    local method="$1"
    local params="${2:-{}}"
    local id="$$-$(date +%s)"
    
    local request=$(jq -n \
        --arg id "$id" \
        --arg method "$method" \
        --argjson params "$params" \
        '{id: $id, method: $method, params: $params}')
    
    # Send request and get response
    echo "$request" | nc -U "$MCP_SOCKET"
}

# List tools
mcp_request "tool.list"

# Execute git status
mcp_request "tool.execute" '{"tool": "git", "args": ["status"]}'
```

## Security Considerations

1. **Authentication**: MCP socket is protected by file permissions
2. **Authorization**: Tools must be registered and enabled
3. **Validation**: All inputs are validated before execution
4. **Sandboxing**: Tools execute in sandboxed environment
5. **Audit**: All MCP requests are logged

## Error Codes

- `-32700`: Parse error
- `-32600`: Invalid request
- `-32601`: Method not found
- `-32602`: Invalid params
- `-32603`: Internal error
- `-32001`: Tool not found
- `-32002`: Tool disabled
- `-32003`: Execution failed

## Best Practices

1. **Use unique request IDs** for tracking
2. **Handle timeouts** appropriately
3. **Validate responses** before use
4. **Log errors** for debugging
5. **Implement retry logic** for transient failures

## Extending MCP

To add custom MCP handlers:

1. Create handler function in your plugin:
```bash
my_plugin_mcp_handler() {
    local operation="$1"
    shift
    
    case "$operation" in
        custom_method)
            # Handle custom method
            echo '{"result": "success"}'
            ;;
        *)
            echo '{"error": "Unknown method", "code": -32601}'
            ;;
    esac
}
```

2. Register handler during plugin initialization:
```bash
echo "myplugin:my_plugin_mcp_handler" >> "$SENTINEL_TOOLS_DIR/mcp_handlers.txt"
```

3. Use namespaced method names:
```json
{
  "method": "myplugin.custom_method",
  "params": {}
}
```