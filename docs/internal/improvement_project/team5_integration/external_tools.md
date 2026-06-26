# VANTAGE External Tools Integration

## Overview

The External Tools Integration module provides a secure, extensible plugin system for integrating third-party tools into the VANTAGE environment. It supports tool discovery, registration, sandboxing, and Model Context Protocol (MCP) integration for AI-assisted operations.

## Architecture

### Core Components

1. **Tool Registry**: JSON-based registry tracking all registered tools and plugins
2. **Plugin System**: Dynamic plugin loader for extending functionality
3. **Sandbox Environment**: Security isolation for external tool execution
4. **MCP Integration**: AI model communication protocol for tool orchestration

### Directory Structure

```
~/.vantage/tools/
├── registry.json          # Tool and plugin registry
├── sandbox/              # Sandboxed tool wrappers
├── plugins/              # Plugin extensions
├── mcp.sock             # MCP communication socket
└── mcp_handlers.txt     # MCP handler registry
```

## Features

### 1. Tool Discovery and Registration

Automatically discover and register tools from system PATH:

```bash
# Discover all tools in PATH
vantage_tool_discover

# Discover specific tool type
vantage_tool_discover /usr/local/bin vcs

# Register a specific tool
vantage_tool_register git /usr/bin/git '{"type": "vcs", "category": "development"}'
```

### 2. Security Sandboxing

All registered tools can be executed through a security sandbox that:
- Validates commands before execution
- Sets resource limits (CPU, memory)
- Logs all executions for audit
- Prompts for confirmation on destructive operations

```bash
# Enable sandboxing (default)
export VANTAGE_TOOLS_SANDBOX_ENABLED=1

# Execute tool through sandbox
vantage_tool_execute git status
```

### 3. Plugin System

Extend functionality with plugins:

```bash
# Load a plugin
vantage_plugin_load git_integration

# List loaded plugins
jq '.plugins' ~/.vantage/tools/registry.json
```

### 4. MCP (Model Context Protocol) Integration

Enable AI models to interact with tools:

```bash
# Enable MCP support
export VANTAGE_MCP_ENABLED=1

# MCP will create a socket at ~/.vantage/tools/mcp.sock
# AI models can send JSON requests to execute tools
```

MCP Request Format:
```json
{
  "method": "tool.execute",
  "params": {
    "tool": "git",
    "args": ["status", "--porcelain"]
  }
}
```

## Included Plugins

### Git Integration Plugin

Enhanced Git workflows with security features:

```bash
# Smart git operations
vantage_git_status        # Enhanced status display
vantage_git_commit        # Commit with templates
vantage_git_workflow feature my-feature  # Feature branch workflow
vantage_git_analyze       # Repository analysis

# Git hooks integration
vantage_git_hooks_init    # Install security hooks
```

### Docker Integration Plugin

Secure Docker container management:

```bash
# Security scanning
vantage_docker_scan nginx:latest

# Safe container execution
vantage_docker_run nginx  # Runs with security options

# Container monitoring
vantage_docker_monitor    # Real-time container stats

# Cleanup utilities
vantage_docker_cleanup safe  # Remove stopped containers
```

### Kubernetes Integration Plugin

Kubernetes cluster management with security focus:

```bash
# Context management
vantage_k8s_context list
vantage_k8s_context switch production

# Security scanning
vantage_k8s_scan all      # Comprehensive security scan

# Safe kubectl operations
vantage_kubectl delete pod my-pod  # Prompts for confirmation

# Resource monitoring
vantage_k8s_monitor pods  # Real-time pod monitoring

# Troubleshooting
vantage_k8s_debug my-pod  # Comprehensive pod debugging
```

## Configuration

### Environment Variables

```bash
# External tools configuration
VANTAGE_TOOLS_DIR="$HOME/.vantage/tools"
VANTAGE_TOOLS_SANDBOX_ENABLED=1
VANTAGE_TOOLS_VERIFICATION=1
VANTAGE_TOOLS_ALLOWED_COMMANDS="git,docker,kubectl,terraform,ansible"

# MCP configuration
VANTAGE_MCP_ENABLED=1
VANTAGE_MCP_TIMEOUT=30

# Plugin-specific settings
DOCKER_SCAN_ENABLED=1
DOCKER_RESOURCE_LIMITS=1
K8S_SECURITY_SCAN=1
```

### Tool Registration

Register tools manually:
```bash
vantage_tool_register terraform /usr/local/bin/terraform '{
  "type": "infrastructure",
  "category": "iac",
  "version": "1.5.0"
}'
```

### Creating Aliases

Create convenient aliases for tools:
```bash
# Create alias with default arguments
vantage_tool_alias tf terraform
vantage_tool_alias k kubectl
vantage_tool_alias d docker
```

## Security Features

### 1. Command Validation
- Checks against allowed command list
- Validates file permissions
- Scans for suspicious arguments

### 2. Resource Limits
- CPU time limits (5 minutes default)
- Memory limits (2GB default)
- Process limits

### 3. Audit Logging
- All tool executions logged
- Timestamps and user information
- Command arguments recorded

### 4. Sandboxing
- Read-only root filesystem
- No new privileges
- Network restrictions (optional)

## Creating Custom Plugins

### Plugin Structure

```bash
#!/usr/bin/env bash
# VANTAGE - My Custom Plugin
# Version: 1.0.0
# Description: Plugin description
# Dependencies: external_tools

# Plugin metadata
VANTAGE_PLUGIN_NAME="my_plugin"
VANTAGE_PLUGIN_VERSION="1.0.0"
VANTAGE_PLUGIN_DESCRIPTION="My custom plugin"

# Plugin functions
my_plugin_function() {
    # Implementation
}

# MCP handler (optional)
my_plugin_mcp_handler() {
    local operation="$1"
    shift
    
    case "$operation" in
        custom_op)
            # Handle MCP request
            ;;
    esac
}

# Initialize plugin
my_plugin_init() {
    # Register tools
    vantage_tool_register "mytool" "/usr/bin/mytool"
    
    # Register MCP handler
    echo "myplugin:my_plugin_mcp_handler" >> "$VANTAGE_TOOLS_DIR/mcp_handlers.txt"
}

# Export functions
export -f my_plugin_function

# Initialize
my_plugin_init
```

### Loading Custom Plugins

1. Place plugin file in `~/.vantage/tools/plugins/`
2. Load with: `vantage_plugin_load my_plugin`

## Best Practices

1. **Security First**: Always enable sandboxing for untrusted tools
2. **Audit Regularly**: Review tool execution logs
3. **Update Registry**: Keep tool versions updated
4. **Test Plugins**: Validate plugins before deployment
5. **Limit Permissions**: Use least privilege principle

## Troubleshooting

### Common Issues

1. **Tool not found**: Ensure tool is in PATH and executable
2. **Permission denied**: Check file permissions and sandbox settings
3. **MCP socket error**: Restart MCP listener with module reload
4. **Plugin load failure**: Check plugin dependencies and syntax

### Debug Mode

Enable debug logging:
```bash
export VANTAGE_MODULE_DEBUG=1
export VANTAGE_LOG_LEVEL=debug
```

### Registry Corruption

Reset registry:
```bash
mv ~/.vantage/tools/registry.json ~/.vantage/tools/registry.json.backup
echo '{"tools": {}, "plugins": {}, "version": "1.0.0"}' > ~/.vantage/tools/registry.json
```

## API Reference

### Core Functions

- `vantage_tool_discover`: Discover tools in PATH
- `vantage_tool_register`: Register a tool
- `vantage_tool_execute`: Execute registered tool
- `vantage_tool_list`: List registered tools
- `vantage_plugin_load`: Load a plugin
- `vantage_tool_alias`: Create tool alias

### MCP Protocol

Methods:
- `tool.execute`: Execute a tool
- `tool.list`: List available tools
- `tool.info`: Get tool information

### Plugin Hooks

- `*_init`: Plugin initialization
- `*_mcp_handler`: MCP request handler

## Future Enhancements

1. **Tool Marketplace**: Central repository for plugins
2. **Remote Tool Execution**: SSH-based tool execution
3. **Tool Chaining**: Complex workflow automation
4. **AI Integration**: Enhanced MCP with LLM capabilities
5. **Policy Engine**: Fine-grained access control