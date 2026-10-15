# OCTOS Roadmap

This document outlines the planned future work for OCTOS.

## Current Release: 0.4.1 (Critical Stabilization & Foundations for 0.5.0)
- [x] **Concurrency & Process Safety**: Stack-allocated `QProcess` per worker thread, lock-free atomic cancellation, leak-free container removal without deadlocks.
- [x] **Deterministic Shutdown Protocol**: Safe 6-step shutdown sequence preventing use-after-free or zombie tasks.
- [x] **Stale Result Invalidation**: Dual revision + pane-specific Job ID tracking with queue coalescing.
- [x] **Docker Security & Resource Budgeting**: Removed 0777 permissions, UID/GID non-root execution, bounded streaming buffers (5MB) for stdout/stderr, strict settings validation.
- [x] **Scalable Myers Diff**: Sub-millisecond to low-millisecond line diffing up to 50k+ lines, line interning, unified patch export, and GUI diff controls.
- [x] **Separated Opcode Reference**: Independent x86-64 and ARM64 instruction databases with robust token filtering and viewport event routing.
- [x] **Compiler Registry & Async Docker Manager**: Explicit metadata, strict language-image pairing, asynchronous image discovery, and safe deletion.
- [x] **0.5.0 Core Foundations**: `CompilationArtifact`, `AssemblyParser` (functions, basic blocks, control flow), and `WorkspaceModel` (multi-file JSON workspace).

## Upcoming Releases

### Version 0.5.0 (Structured Artifacts, Assembly AST & Multi-File Workspaces)
- [x] Core models (`WorkspaceModel`, `AssemblyParser`, `CompilationArtifact`) implemented in `octos_core`
- [x] Multi-file workspace UI (project explorer, editor tabs, per-file dirty state, profiles). Open: target selection UI, CMake/Makefile generation
- [x] CFG data (functions, blocks, edges) and analysis panel for x86-64 and AArch64. Open: graphical CFG visualizer
- [ ] LLVM IR extraction and viewer alongside assembly output
- [ ] Complete integration and verification of Ada GNAT and C# Mono container pipelines

### Version 0.6.0 (Packaging & Platform Polish)
- **Platform Packaging**: Official Linux AppImage/Flatpak, macOS `.dmg`, and native Windows installers.
- **Local Toolchain Fallback**: Optional non-containerized execution for systems without Docker.
- **Performance Profiling**: Basic cycle counting and estimation metrics.
