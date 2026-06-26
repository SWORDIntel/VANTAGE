# VANTAGE AI Assistant Context

This directory contains comprehensive context and knowledge for AI assistants and developers working on VANTAGE.

## Directory Structure

```
.claude/
├── README.md                    # This file - overview and navigation
├── context/                     # Project context and current state
│   ├── project_overview.md      # High-level project description
│   ├── current_state.md         # Current implementation status
│   └── development_history.md   # History of major changes
├── improvements/                # Documented improvements and their implementations
│   ├── implemented/             # Successfully implemented features
│   ├── planned/                 # Planned but not yet implemented
│   └── rejected/                # Considered but rejected ideas
├── guides/                      # Development guides and how-tos
│   ├── module_development.md    # How to create new modules
│   ├── python_integration.md    # Python component guidelines
│   ├── testing_guide.md         # Testing procedures
│   └── debugging_guide.md       # Common issues and solutions
├── architecture/                # System architecture documentation
│   ├── system_design.md         # Overall system architecture
│   ├── module_system.md         # Module loading and management
│   ├── security_model.md        # Security considerations
│   └── performance.md           # Performance optimization notes
├── knowledge/                   # Lessons learned and best practices
│   ├── design_decisions.md      # Key design decisions and rationale
│   ├── pitfalls.md              # Common pitfalls to avoid
│   ├── best_practices.md        # Established best practices
│   └── troubleshooting.md       # Known issues and solutions
└── templates/                   # Templates for common tasks
    ├── module_template.module   # Template for new modules
    ├── python_tool_template.py  # Template for Python tools
    ├── test_template.sh         # Template for test scripts
    └── documentation_template.md # Template for documentation

## Quick Start for AI Assistants

1. **Understanding the Project**: Start with `context/project_overview.md`
2. **Current State**: Review `context/current_state.md` for what's implemented
3. **Making Changes**: Follow guides in `guides/` directory
4. **Architecture**: Understand the system via `architecture/system_design.md`
5. **Best Practices**: Review `knowledge/best_practices.md` before making changes

## Key Principles

1. **Modularity**: Everything is a module - see `architecture/module_system.md`
2. **Security First**: All changes must consider security - see `architecture/security_model.md`
3. **Performance**: Bash startup time is critical - see `architecture/performance.md`
4. **Compatibility**: Must work on various Linux distributions
5. **Documentation**: Every feature must be documented

## Common Tasks

- **Adding a Module**: Use `templates/module_template.module` and follow `guides/module_development.md`
- **Adding Python Tool**: Use `templates/python_tool_template.py` and follow `guides/python_integration.md`
- **Fixing Issues**: Check `knowledge/troubleshooting.md` first
- **Testing Changes**: Follow `guides/testing_guide.md`

## Important Files to Know

- `install.sh` - Main installation script (be very careful with changes)
- `bash_modules` - Core module loading system
- `bashrc.postcustom` - Where user customizations are loaded
- `scripts/vantage_postinstall_check.sh` - Verification script (run after changes)

## Development Workflow

1. Understand the context (this directory)
2. Make changes following the guides
3. Test thoroughly using provided test scripts
4. Update relevant documentation
5. Run `scripts/vantage_postinstall_check.sh` to verify

## Contact and Resources

- Project: VANTAGE (Secure ENhanced Terminal INtelligent Layer)
- Purpose: Comprehensive bash environment for cybersecurity professionals
- Language: Primarily Bash with Python for ML/AI features