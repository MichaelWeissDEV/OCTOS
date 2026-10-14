# OCTOS Roadmap

This document outlines the planned future work for OCTOS.

## Current Release: 0.3.0 (Stabilization & Core Foundation)
- [x] Decouple core logic into `octos_core` library
- [x] Automated test suite integrated via CTest and Qt Test
- [x] Strategy-based compiler driver architecture (`ILanguageStrategy`)
- [x] Security-hardened Docker execution sandbox (`--network none`, resource limits, dropped capabilities, no-new-privileges)
- [x] Revision-aware job management with automatic invalidation and cancellation
- [x] Fixed language-specific compilation pipelines (GCC, Clang, Rust, Java, Python)
- [x] Full document state tracking (Save, Save As, dirty tracking, recent files)
- [x] Centralized persistent settings (`AppSettings`)

## Upcoming Releases

### Version 0.4.0 (Advanced Assembly Analysis & UX)
- [x] **Assembly Diff View**: Side-by-side graphical diff comparison highlighting instruction differences across compiler versions or optimization levels.
- [x] **Architectural Tooltips**: Hover over x86/ARM assembly mnemonics to view instruction summary, flags affected, and syntax examples.
- [x] **Enhanced Syntax Highlighters**: Dedicated highlighters for Rust and Python in the source editor with dynamic language detection.
- [ ] **Multi-File / Multi-Source Support**: Support compiling projects with header files and small modules.

### Version 0.5.0 (Extended Toolchains & Architectures)
- **Ada GNAT Support**: Complete integration and verification of the Ada GNAT compilation pipeline.
- **Go and Zig Support**: Native assembly extraction for Go (`go build -gcflags -S`) and Zig (`zig build-exe -femit-bin`).
- **Cross-Compilation Targets**: ARM64, RISC-V, and AVR cross-compiler container images with simulated instruction output.
- **LLVM IR / Tree Visualization**: View LLVM Intermediate Representation and compiler optimization passes.

### Version 0.6.0 (Packaging & Platform Polish)
- **Platform Packaging**: Official Linux AppImage/Flatpak, macOS `.dmg`, and native Windows installers.
- **Local Toolchain Fallback**: Optional non-containerized execution for systems without Docker.
- **Performance Profiling**: Basic cycle counting and estimation metrics.
