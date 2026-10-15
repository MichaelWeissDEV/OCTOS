# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Development work on top of the `0.4.1` state. **No version has been bumped and nothing has been
released**; this section only records what changed in the working tree.

### Changed — stability
- **`JobManager` is a real bounded scheduler** in front of the `QThreadPool`: at most `maxParallelJobs` jobs in
  flight, at most one waiting job per pane (replaced in place), a bounded total queue, no unstarted
  `QRunnable`s piling up in the pool. Slots are accounted on the GUI thread.
- **Shutdown waits for the workers for real** (worker counter instead of a fixed `waitForDone`), re-sends
  cancellation, returns `false` if workers did not terminate and then keeps shared state alive for them.
- **Docker backend:** `cancel()` never blocks (flag only); the container stays registered until its cleanup
  has run; cleanup is idempotent, refuses non-`octos_` names, and failures are tracked as leaked containers
  and retried; no docker process runs under the backend mutex; cancelling a finished job no longer leaks an
  entry.
- **Bug fixed — truncation flag:** `artifactTruncated` was reset by `processOutput()`; it is now preserved,
  the UI gets a warning and the result is not `isComplete()`.
- **Output limits:** 64-bit arithmetic, separate stdout/stderr limits, UTF-8-safe cuts, flood detection
  (kill after 4× the limit), source/artifact size limits, artifact read refuses symlinks leaving the
  sandbox. `docker run` now uses `--pull=never` (no implicit downloads), `--memory-swap`, `--ipc none`, an
  always-bounded `/tmp` tmpfs and a file-size ulimit (exit 153 → *Limit exceeded*).
- `DockerExecutionConfig::fromSettings()` validates per field instead of resetting everything.

### Changed — analysis
- **Diff engine rewritten:** linear-space Myers (O(N+M) memory), explicit budgets, cancellation inside the
  algorithm, incomplete results clearly flagged, dialect-aware normalisation (ARM64 `#42` is not a comment),
  label renaming by first appearance, valid unified diff export (hunks, ranges, no-newline marker; round trip
  verified with the system `patch`).
- **`DiffDialog` computes asynchronously** (cancellable, newest request wins, safe on close) and uses a virtual
  `QAbstractTableModel`.
- **`AssemblyParser` rewritten:** `.type/.size/.globl`-based function boundaries, section tracking, jump-target
  based basic blocks, an explicit CFG (`Taken`/`Fallthrough`/`Unconditional`/`IndirectUnknown`), structured
  operands (original text always kept), `.file`/`.loc` source mapping, metrics. ~5× faster after a regex fix.
- New **Assembly Analysis** panel (functions, blocks, successors, statistics, navigation).

### Changed — compilers & registry
- **Rust:** explicit crate type (`Auto` → library when there is no `main`), deduplicated flags, edition 2021.
- **Java:** disassembles all generated classes (packages, inner classes). **Python:** `SyntaxError`s now
  produce diagnostics; output is labelled CPython bytecode.
- **Verification states** replace the old "Verified unless experimental": image recognised / plausible /
  verified (in-image probe: tools, version, architecture, image ID) / failed. Image *ownership* is tracked
  explicitly; deleting a foreign image needs extra confirmation; `docker images` is never queried
  synchronously in GUI paths and query failures are reported.

### Added — projects, artifacts, UI
- Validated, atomically saved project files (`QSaveFile`), format gate, safe relative paths, symlink-safe
  materialisation; `ProjectController` (per-file dirty state, single-file saves); editor tabs, project
  explorer, find/replace, auto-indent; user-level **compiler profiles** and *Compare Two Profiles*;
  extra project files travel into the sandbox.
- Typed artifacts with ids, provenance, checksum and explicit location lifecycle; artifact selector,
  structured diagnostics list, editor error markers, per-type highlighters, *Save…*, compile metadata artifact.
- Architecture-aware compiler flags (no `-masm=` on aarch64).
- Status badges distinguish compiler / Docker / limit / internal failures; warnings and truncation are shown.
- **Theme tokens:** every widget takes its colours from the theme (no hard-coded dark colours); contrast is
  tested for all six themes. Dockable layout persisted between sessions, *Reset Layout*.

### Tests & tooling
- Test runs are **isolated from the user's preferences** (`OCTOS_SETTINGS_DIR`, `OCTOS_TEST_MAIN`).
- ThreadSanitizer runs without suppressions (explicit hand-off annotations, mutex-protected result inbox).
- New suites for the scheduler, Docker backend (scripted fake docker + real daemon flags), parser/CFG on real
  Clang output, diff engine and dialog, workspace/project controller, registry/probe, themes and main window
  (offscreen screenshots), fuzz-property tests; libFuzzer targets; `octos_bench`; ThreadSanitizer,
  AddressSanitizer and UBSan runs; weak assertions (`|| true`, mock-only checks) replaced.

### Known limits
See `docs/limitations.md`. Notably: no real GNU GCC corpus, x86-64 only parsing, Docker is not a complete
sandbox for hostile code.

## [0.4.1] - 2026-10-14

### Added
- **Myers Assembly Diff Processor**:
  - Scalable $O(ND)$ Myers diff algorithm replacing the $O(NM)$ full LCS matrix; handles 50,000+ lines in under 60ms.
  - Line interning, common prefix/suffix trimming, configurable step budgets, and cancellation support.
  - Configurable normalization: whitespace suppression, comment stripping, and label normalization.
  - Unified diff patch generation with export to `.patch` file or clipboard.
  - Diff view improvements: interactive pane selector (Pane A vs Pane B), divergent artifact warning banner, diff row search with forward/backward navigation, and "Show Only Differences" toggle.
- **Architecture-Separated Opcode Reference**:
  - Disjoint opcode dictionaries for x86-64 and ARM64.
  - Strict mnemonic token extraction ignoring operands, labels, directives, and comments.
  - Viewport event filter in `CodeEditor` ensuring reliable `QEvent::ToolTip` delivery on Qt 6.
- **Compiler Registry & Modernized Docker Manager**:
  - Central `CompilerRegistry` with explicit compiler metadata, syntax flavors, artifact types, and language compatibility validation.
  - Asynchronous Docker daemon reachability checking and non-blocking image inventory.
  - Safe image management protecting unmanaged user images from accidental deletion.
- **Technical Foundations for 0.5.0**:
  - **Structured Compilation Artifacts (`CompilationArtifact`)**: First-class artifact model supporting Assembly, LLVM IR, Bytecode, CIL, and diagnostics with size and MIME tracking.
  - **Assembly Parser (`AssemblyParser`)**: Parsing of x86-64 assembly into structured functions, basic blocks, jump links, and categorized operands.
  - **Multi-File Workspace Model (`WorkspaceModel`)**: Project workspace data model supporting multiple source files, compiler profiles, build targets, and JSON persistence (`octos-project-v1`).
- **Comprehensive Unit & Integration Test Suites**:
  - Added `test_parser_and_workspace` validating assembly parsing, basic blocks, frequency counting, and workspace JSON round-trip.
  - Added end-to-end multi-language test slots in `test_docker_integration` with safe Docker-offline detection.
  - Automated offscreen platform rendering (`QT_QPA_PLATFORM=offscreen`) in GitHub Actions CI.

### Fixed & Stabilized
- **Thread Safety in Compilation Process Execution**:
  - Replaced cross-thread `QProcess` pointer manipulation with thread-confined worker stack processes.
  - Lockless cancellation signalling via atomic flags; container cleanup via `docker kill`/`rm` without holding internal backend mutexes.
- **Deterministic Shutdown Protocol**:
  - Guaranteed orderly shutdown: new job rejection, queue clearing, cancellation signalling, container cleanup, and bounded worker thread synchronization.
- **Stale Compilation Result Discarding**:
  - Track both document revision and per-pane Job IDs to eliminate race conditions from rapid flag toggling or recompilations.
  - Coalescing job submission queue that cancels obsolete queued or running tasks immediately.
- **Docker Security & Host Isolation**:
  - Removed world-writable permissions (`0777`) on temp directories; enforced strict `0750` permissions with UID/GID container user mapping.
  - Streaming bounded buffers for stdout/stderr preventing host memory exhaustion on huge compiler output.
  - Rigorous input validation for Docker resource limits (memory, CPU, PID bounds) synchronized from `AppSettings`.

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
