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
│  - Bounded scheduler queue   - Revision invalidation   │
│  - Job state transitions     - Cancellation / shutdown │
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
   - The job waits in the scheduler queue (at most one waiting job per pane, bounded total) and is only handed to the thread pool while fewer than `maxParallelJobs()` jobs are in flight. See [Job Manager](job-manager.md).
4. **Execution (`DockerExecutionBackend`)**:
   - The strategy matching the language and compiler formats the exact compiler invocation arguments.
   - Host temporary input and output directories are prepared.
   - The container is executed using strict resource and isolation flags (see [Docker Backend](docker-backend.md)); outputs are read back under hard size limits.
5. **Result Handling**:
   - When a job completes, `JobManager::handleTaskFinished()` checks if the job's revision matches the latest revision for that pane.
   - If the job was superseded by newer typing, its result is safely discarded.
   - Otherwise, the `CompilationResult` is dispatched to `MainWindow`.
6. **Presentation**:
   - `presentResult()` decides the status badge and notice (compiler vs. Docker vs. internal error, truncation, cleanup warnings).
   - The pane lists the result's typed artifacts. Assembly goes through `AssemblyTextProcessor` (display filters, demangling, line mappings); other artifact types are shown as-is with their own highlighter; diagnostics are a structured list.
   - The raw assembly is also parsed once by `AssemblyParser` (worker thread) for the Analysis panel. See [Assembly Parser](assembly-parser.md).

## Modules

| Library / area | Contents |
| --- | --- |
| `octos_core` | strategies, `JobManager`, Docker backend and manager, `CompilerRegistry` + probe, `AssemblyParser`, diff engine, `Workspace`, `ProfileStore`, `ProjectController` |
| `octos_ui` | `MainWindow` (+ `MainWindowProject.cpp`), `CompilerOutputPane`, `AnalysisPanel`, `DiffDialog`, `ProjectExplorerDock`, `CodeEditor`, `Theme` (design tokens), highlighters |
| `OCTOS` | `main.cpp` only |

The application and all GUI tests link the same `octos_ui` library.

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
