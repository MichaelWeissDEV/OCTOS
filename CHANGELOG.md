# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - Unreleased

### Added
- Assembly output for all supported languages
- Docker-based containerized compilation
- Multi-language snippet system
- Compiler manager with Docker image management
- Advanced assembly filtering and annotation
- Syntax highlighting for assembly code
- Code editor with line numbers and highlighting
- Recent files tracking system
- Dark theme UI with Qt 6

### Changed
- Refactored compiler management system
- Improved error message display
- Optimized Docker compilation for speed
- Better assembly output formatting

### Fixed
- Rust compilation with proper `--emit asm` flags
- Python syntax checking with py_compile
- Java bytecode generation with javap
- C/C++ error display in output pane

### Known Issues
- First compilation of each language takes time (Docker image pull)
- Some edge cases in assembly filtering not yet handled
- Windows support is experimental; WSL2 recommended
- Limited testing on macOS

## [0.1.0] - 2025-12-13

### Added
- Initial open source release
- Support for 6 programming languages
- Multi-window compilation interface
- Assembly visualization
- Docker containerization

---

## How to Report Changes

When making changes to this project:

1. Add an entry under `[Unreleased]` in the appropriate section
2. Use these categories:
   - **Added**: New features
   - **Changed**: Changes in existing functionality
   - **Deprecated**: Soon-to-be removed features
   - **Removed**: Removed features
   - **Fixed**: Bug fixes
   - **Security**: Security improvements
3. Include relevant issue/PR numbers

## Version Numbering

use [Semantic Versioning](https://semver.org/):
- **MAJOR**: Breaking changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes

---

## Release History

### 2025-12-13
- Initial open source release on GitHub
- Converted from internal pet project
- 6 languages supported
- Docker compilation backend

---

For more information, see the [README](README.md).
