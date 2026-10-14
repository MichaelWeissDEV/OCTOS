# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2026-10-14

### Added
- **Assembly Diff View (`DiffDialog` & `AssemblyDiffProcessor`)**:
  - Side-by-side assembly comparison view with LCS-based line matching.
  - Visual addition, deletion, and line numbering cues for compiler output comparisons.
  - Diff view action button automatically enabled when 2+ compiler panes are open.
- **Architectural Opcode Tooltips (`OpcodeReference`)**:
  - Embedded instruction reference dictionary for x86 and ARM64 instructions.
  - Rich HTML tooltips on hovering over assembly mnemonics showing instruction summary, flags affected, and syntax examples.
- **Multi-Language Source Syntax Highlighting**:
  - Added dedicated syntax highlighters for Rust (`RustHighlighter`) and Python (`PythonHighlighter`).
  - Dynamic switching of source syntax highlighting based on file extension and selected compiler pane language.
- **Automated Unit Tests**:
  - Added `test_diff_and_highlighters` suite validating diff computation, opcode reference queries, and language highlighters.

## [0.3.0] - 2026-10-14

### Added
- **Core Decoupling (`octos_core`)**: Separated compilation strategies, Docker execution backend, job management, assembly processing, and settings from UI widgets into a reusable static library.
- **Automated Test Infrastructure**: Complete test suite integrated with CMake and CTest:
  - `test_compiler_strategies`: Validates command-line generation, syntax flavors, user flag sanitization, and diagnostic parsing.
  - `test_assembly_processor`: Tests directive filtering, comment stripping, label cleanups, and source-to-assembly line mapping.
  - `test_job_manager`: Verifies job lifecycle states (`Queued`, `Starting`, `Running`, `Completed`, `Failed`, `Cancelled`, `TimedOut`), revision invalidation, and cancellation.
  - `test_docker_security`: Verifies network isolation (`--network none`), memory/CPU/PID resource limits, capability dropping, and volume sandboxing.
  - `test_settings_and_files`: Tests configuration persistence, boundary enforcement, and recent files management.
  - `test_docker_integration`: Integration tests with Docker containers that cleanly report `QSKIP` when the Docker daemon is offline.
- **Language Strategy Architecture (`ILanguageStrategy`)**:
  - Independent compilation strategies for GCC, Clang, Rust, Java, Python, C#, and Ada.
  - Central `LanguageStrategyRegistry` providing strategy lookups.
- **Job Lifecycle & Concurrency Management (`JobManager`)**:
  - Revision tracking per output pane to prevent stale results from overwriting newer user edits.
  - Automatic cancellation of superseded compilation tasks.
  - Bounded thread pool and configurable parallel execution limits.
  - Safe application shutdown ensuring no process or container leaks.
- **Docker Security Hardening**:
  - Strict sandboxing: `--network none`, `--memory 512m`, `--cpus 1.5`, `--pids-limit 128`, `--security-opt=no-new-privileges`, `--cap-drop=ALL`.
  - Read-only input mounts (`/input:ro`) and controlled writable output mounts (`/output:rw`).
  - Strict user argument sanitization preventing shell injection and output file hijacking.
- **File Management & Usability**:
  - Proper distinction between `Save` and `Save As...`.
  - Document dirty state tracking with unsaved changes prompts on New, Open, and Exit.
  - Dynamic recent files menu with persistence in `RecentFilesManager`.
  - Centralized persistent application settings via `AppSettings`.
- **Continuous Integration**: Local GitHub Actions CI workflow for Linux and macOS builds and tests.

### Changed
- Refactored `DockerCompilerManager` to delegate compilation execution to `JobManager` while maintaining backward compatibility.
- Fixed `getCategoryRegistry()`: Clang category now correctly points to `silkeh/clang` instead of GCC.
- Cleaned up GNU line markers and updated deprecated Qt 6 calls to standard `std::as_const()`.
- Unified application version to `0.3.0` across CMake, `main.cpp`, and documentation.

### Fixed
- **Rust**: Fixed illegal injection of GCC options (`-masm=intel`, `-fno-asynchronous-unwind-tables`) into `rustc`; correctly applies `--emit=asm`, LLVM syntax flavor args, and Rust debug flags.
- **Python**: Resolved `Permission denied` failures caused by read-only `/input` mounts; python bytecode is now compiled strictly to `/output/source.pyc` and disassembled via `dis`.
- **Java**: Fixed class file detection and bytecode extraction; eliminated shell command injection by parameterizing compilation scripts.
- **Assembly Processing**: Fixed comment stripping to support C/C++ style single-line comments (`//`) alongside `#` and `;`.
- **Security**: Removed unused and hazardous `PackageManager` with privileged `pkexec sh -c` execution.

## [0.2.0] - 2026-03-01

### Added
- Assembly output for supported languages
- Docker-based containerized compilation
- Multi-language snippet system
- Compiler manager with Docker image management
- Advanced assembly filtering and annotation
- Code editor with line numbers and highlighting
- Recent files tracking system
- Dark theme UI with Qt 6

### Changed
- Refactored compiler management system
- Improved error message display

## [0.1.0] - 2025-12-13

### Added
- Initial open source release
- Support for programming languages
- Multi-window compilation interface
- Docker compilation backend
