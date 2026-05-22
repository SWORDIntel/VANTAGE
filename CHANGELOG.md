# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project aims to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Canonical local test runner via `make test-fast`, `make test`, and `make test RUN_OPTIONAL=1`
- CI workflow and dedicated requirements manifests for CI and optional Markov dependencies
- Archived internal documentation area under `docs/internal/`

### Changed
- Fixed installer control flow for unattended and headless operation
- Hardened module loading, health-check, logging, Python bridge, and Markov integration paths
- Standardized module discovery so helper scripts are no longer treated as loadable modules
- Reworked public-facing documentation to match the current install, validation, and repository layout

### Removed
- Tracked backup artifacts, stale helper copies, and committed bytecode/cache noise from the repository
- Root-level internal progress and audit notes from the primary public surface
