# Architecture Overview

OCTOS follows a decoupled architecture separating UI presentation from compilation logic, job scheduling, and container execution.

## Core Architecture (`octos_core`)

All core logic resides in the `octos_core` library, enabling headless testing and clean dependency boundaries:

```
┌────────────────────────────────────────────────────────┐
│                      GUI Layer                         │
│  MainWindow, CompilerOutputPane, Highlighters, Theme   │
└───────────────────────────┬────────────────────────────┘
                            │ (CompilationRequest / Qt Signals)
                            ▼
┌────────────────────────────────────────────────────────┐
│                     JobManager                         │
│  - ThreadPool scheduling     - Revision invalidation   │
│  - Job state transitions     - Cancellation tracking   │
└───────────────────────────┬────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
┌───────────────────────────┐ ┌───────────────────────────┐
│   Language Strategies     │ │   Execution Backend       │
│ - GCC / Clang Strategy    │ │ - DockerExecutionBackend  │
│ - Rust Strategy           │ │   (Resource limits,       │
│ - Java Strategy           │ │    network isolation)     │
│ - Python Strategy         │ │ - MockExecutionBackend    │
│ - C# / Ada Strategy       │ │   (Automated test suite)  │
└───────────────────────────┘ └───────────────────────────┘
```

## Data Flow

1. **User Edit**:
   - As the user types in `CodeEditor`, the `m_inputRevision` counter increments.
   - The debounce timer (default 300ms, configurable via `AppSettings`) delays compilation until typing pauses.
2. **Request Dispatch**:
   - `MainWindow::compileAllPanes()` constructs a `CompilationRequest` for each open pane with the pane ID, input revision, source code, compiler image, language, syntax flavor, and user flags.
3. **Job Scheduling (`JobManager`)**:
   - `JobManager::submitJob()` registers the request with a unique `jobId`.
   - Any prior jobs running for the same `paneId` are immediately marked cancelled and their backend processes interrupted.
   - The job is queued in a bounded thread pool (`QThreadPool`).
4. **Execution (`DockerExecutionBackend`)**:
   - The strategy matching the language and compiler formats the exact compiler invocation arguments.
   - Host temporary input and output directories are prepared.
   - The container is executed using strict resource and isolation flags (`--network none`, `--memory`, `--cpus`, `--pids-limit`, `--security-opt=no-new-privileges`, `--cap-drop=ALL`).
5. **Result Handling**:
   - When a job completes, `JobManager::handleTaskFinished()` checks if the job's revision matches the latest revision for that pane.
   - If the job was superseded by newer typing, its result is safely discarded.
   - Otherwise, the `CompilationResult` is dispatched to `MainWindow`.
6. **Assembly Processing**:
   - `AssemblyTextProcessor::processAssembly()` filters out unnecessary directives, demangles C++ symbols, and computes line mappings.
   - The filtered assembly is rendered in the `CompilerOutputPane`.

## Adding a New Compiler or Language

To add support for a new language:
1. Create a strategy class inheriting from `ILanguageStrategy` (e.g. `GoStrategy`).
2. Implement:
   - `buildCompilerArgs()`: construct non-shell argument lists for the compiler.
   - `sourceFileName()` and `outputFileName()`.
   - `supportsSyntaxFlavor()`: return whether Intel/AT&T is supported.
   - `sanitizeUserFlags()`: filter out hazardous or invalid flags.
   - `processOutput()`: parse output artifacts and diagnostics.
3. Register the strategy in `LanguageStrategyRegistry`.
4. Add the language enum to `CompilerEnums.h` if needed.
5. Add unit tests in `tests/unit/test_compiler_strategies.cpp`.
