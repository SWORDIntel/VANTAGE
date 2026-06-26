# VANTAGE Module System Documentation

## Overview

The VANTAGE module system is a sophisticated framework for extending bash functionality through self-contained, dynamically loadable modules. This document details how the module system works, how to create modules, and how modules interact with the core system.

## Module Architecture

### Module Loading Process

1. **Discovery Phase**
   - The module loader scans `bash_modules.d/` directory
   - Identifies all `.module` files
   - Parses module metadata

2. **Dependency Resolution**
   - Builds dependency graph
   - Determines load order
   - Detects circular dependencies

3. **Loading Phase**
   - Sources modules in resolved order
   - Executes initialization functions
   - Registers module capabilities

4. **Post-Load Phase**
   - Runs post-initialization hooks
   - Verifies module health
   - Updates module registry

### Module Structure

Each module follows a standard structure:

```bash
#!/bin/bash
# Module: module_name
# Description: Brief description of module functionality
# Version: 1.0.0
# Dependencies: dep1,dep2
# Author: Author Name
# Tags: tag1,tag2,tag3

# Module initialization
module_name_init() {
    # Initialization code
}

# Module functionality
module_name_function() {
    # Function implementation
}

# Module cleanup (optional)
module_name_cleanup() {
    # Cleanup code
}

# Register module
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    module_name_init
fi
```

## Core Modules

### 1. System Management Modules

#### module_manager.module
Central module management system that handles:
- Module registration and tracking
- Dependency resolution
- Load order management
- Module health monitoring

#### config_cache.module
Configuration caching system:
- Caches parsed configurations
- Reduces startup time
- Manages configuration updates

#### logging.module
Comprehensive logging framework:
- Multiple log levels
- Rotation and archival
- Performance metrics logging

### 2. Security Modules

#### shell_security.module
Shell hardening and security features:
- Command validation
- Input sanitization
- Security policy enforcement

#### hmac.module
HMAC-based verification system:
- Module integrity checking
- Configuration validation
- Secure communication

#### obfuscate.module
Code and data obfuscation utilities:
- String obfuscation
- Path hiding
- Sensitive data protection

### 3. Enhancement Modules

#### autocomplete.module
Advanced autocomplete features:
- Context-aware suggestions
- Learning from user behavior
- Custom completion handlers

#### fuzzy_correction.module
Fuzzy command correction:
- Typo detection and correction
- Similar command suggestions
- Learning from corrections

#### fzf.module
FZF (Fuzzy Finder) integration:
- Enhanced file searching
- Command history search
- Interactive selection

### 4. ML/AI Modules

#### vantage_ml.module
Core ML functionality:
- Model loading and management
- Inference pipeline
- Feature extraction

#### vantage_chat.module
LLM chat integration:
- Local model support
- Context management
- Conversation history

#### vantage_context.module
Context awareness system:
- Working directory tracking
- Command context analysis
- Project detection

#### vantage_cybersec_ml.module
Cybersecurity-focused ML features:
- Threat detection
- Anomaly detection
- Security recommendations

### 5. OSINT Modules

#### vantage_osint.module
OSINT data collection:
- Multiple data source integration
- Data aggregation
- Result formatting

#### vantage_gitstar.module
GitHub repository analysis:
- Repository discovery
- Code analysis
- Security scanning

### 6. Development Tools

#### distcc.module
Distributed compilation support:
- Automatic host discovery
- Load balancing
- Compilation caching

#### hashcat.module
Hashcat integration:
- Rule management
- Session handling
- Performance optimization

#### project_suggestions.module
Project-specific suggestions:
- Project type detection
- Context-aware commands
- Custom workflows

## Module Development Guide

### Creating a New Module

1. **Create Module File**
   ```bash
   touch bash_modules.d/my_module.module
   chmod +x bash_modules.d/my_module.module
   ```

2. **Add Module Header**
   ```bash
   #!/bin/bash
   # Module: my_module
   # Description: My custom module
   # Version: 1.0.0
   # Dependencies: logging
   # Author: Your Name
   # Tags: custom,utility
   ```

3. **Implement Core Functions**
   ```bash
   my_module_init() {
       # Initialize module
       log_info "My module initialized"
   }
   
   my_module_main_function() {
       # Main functionality
       echo "Hello from my module!"
   }
   ```

4. **Add Module Registration**
   ```bash
   # Register module
   if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
       my_module_init
   fi
   ```

### Module Best Practices

1. **Naming Conventions**
   - Use lowercase with underscores
   - Prefix all functions with module name
   - Use descriptive names

2. **Error Handling**
   - Always check return codes
   - Provide meaningful error messages
   - Fail gracefully

3. **Performance**
   - Lazy load heavy resources
   - Cache expensive operations
   - Minimize startup impact

4. **Documentation**
   - Document all public functions
   - Include usage examples
   - Maintain changelog

### Module Configuration

Modules can have their own configuration:

```bash
# In module
my_module_config() {
    # Default configuration
    MY_MODULE_ENABLED=${MY_MODULE_ENABLED:-true}
    MY_MODULE_TIMEOUT=${MY_MODULE_TIMEOUT:-30}
    MY_MODULE_CACHE_DIR=${MY_MODULE_CACHE_DIR:-"$HOME/.cache/my_module"}
}

# User can override in bashrc.postcustom
export MY_MODULE_TIMEOUT=60
```

### Module Dependencies

Declaring dependencies:

```bash
# Module: advanced_module
# Dependencies: logging,config_cache,fuzzy_correction

advanced_module_init() {
    # Dependencies are guaranteed to be loaded
    log_info "Initializing advanced module"
    config_cache_get "my_setting"
}
```

### Module Communication

Modules can communicate through:

1. **Shared Functions**
   ```bash
   # Module A exports
   module_a_get_data() {
       echo "data from module A"
   }
   
   # Module B uses
   data=$(module_a_get_data)
   ```

2. **Events**
   ```bash
   # Module A triggers event
   trigger_event "data_updated" "new_data"
   
   # Module B listens
   on_event "data_updated" module_b_handle_update
   ```

3. **Shared State**
   ```bash
   # Using module registry
   module_registry_set "my_module.state" "active"
   state=$(module_registry_get "my_module.state")
   ```

## Module Testing

### Unit Testing

Create test file: `tests/test_my_module.sh`

```bash
#!/bin/bash
source ../bash_modules.d/my_module.module

test_my_module_init() {
    my_module_init
    assert_equals $? 0 "Module initialization failed"
}

test_my_module_function() {
    result=$(my_module_main_function)
    assert_contains "$result" "Hello" "Function output incorrect"
}

# Run tests
run_tests
```

### Integration Testing

Test module interactions:

```bash
test_module_integration() {
    # Load multiple modules
    source bash_modules.d/module_a.module
    source bash_modules.d/module_b.module
    
    # Test interaction
    module_a_init
    module_b_init
    
    # Verify behavior
    assert_true "module_b_uses_module_a"
}
```

## Module Distribution

### Packaging Modules

1. **Create Module Package**
   ```bash
   tar -czf my_module.tar.gz \
       my_module.module \
       my_module.conf \
       README.md
   ```

2. **Include Metadata**
   ```json
   {
       "name": "my_module",
       "version": "1.0.0",
       "description": "My custom module",
       "dependencies": ["logging"],
       "vantage_version": ">=2.0.0"
   }
   ```

### Installing External Modules

```bash
# Download and extract
wget https://example.com/modules/cool_module.tar.gz
tar -xzf cool_module.tar.gz -C ~/.vantage/modules/

# Install
vantage module install cool_module

# Enable
vantage module enable cool_module
```

## Module Security

### Security Considerations

1. **Input Validation**
   - Sanitize all user input
   - Validate file paths
   - Check command arguments

2. **Privilege Management**
   - Never run as root unless necessary
   - Drop privileges when possible
   - Use sudo sparingly

3. **Code Signing**
   - Sign modules with GPG
   - Verify signatures on load
   - Maintain trust chain

### Security Module Template

```bash
#!/bin/bash
# Module: secure_module
# Security: signed
# Signature: -----BEGIN PGP SIGNATURE-----...

secure_module_init() {
    # Verify environment
    [[ $EUID -eq 0 ]] && {
        log_error "Module should not run as root"
        return 1
    }
    
    # Validate configuration
    validate_config || return 1
    
    # Initialize securely
    secure_module_setup
}

secure_module_validate_input() {
    local input="$1"
    # Sanitize input
    input=$(printf '%q' "$input")
    # Validate format
    [[ "$input" =~ ^[a-zA-Z0-9_-]+$ ]] || return 1
    echo "$input"
}
```

## Troubleshooting

### Common Issues

1. **Module Not Loading**
   - Check file permissions
   - Verify dependencies
   - Review module logs

2. **Dependency Conflicts**
   - Check version requirements
   - Review load order
   - Use module isolation

3. **Performance Issues**
   - Profile module startup
   - Optimize initialization
   - Use lazy loading

### Debug Mode

Enable module debugging:

```bash
export VANTAGE_MODULE_DEBUG=1
export VANTAGE_MODULE_TRACE=1
```

### Module Diagnostics

```bash
# Check module status
vantage module status my_module

# Verify module integrity
vantage module verify my_module

# Show module dependencies
vantage module deps my_module

# Profile module performance
vantage module profile my_module
```

## Future Enhancements

1. **Module Marketplace**
   - Central repository
   - Rating system
   - Automatic updates

2. **Module Sandboxing**
   - Resource limits
   - Capability restrictions
   - Network isolation

3. **Module Clustering**
   - Distributed modules
   - Load balancing
   - Fault tolerance

## Conclusion

The VANTAGE module system provides a powerful and flexible framework for extending bash functionality. By following the guidelines and best practices outlined in this document, developers can create robust, secure, and efficient modules that enhance the VANTAGE experience.