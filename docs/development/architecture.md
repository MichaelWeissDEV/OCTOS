# Architecture Overview

OCTOS separates *presentation* (`octos_ui`) from *logic* (`octos_core`). Core has no widgets, so everything
important can be tested headlessly.

## Layers

```
┌───────────────────────────────── octos_ui ──────────────────────────────────┐
│ MainWindow · CompilerOutputPane · AnalysisPanel · DiffDialog · Project dock │
│ CodeEditor · highlighters · Theme (tokens, roles)                            │
└──────────────┬───────────────────────────────────────────────┬──────────────┘
   CompilationRequest / jobFinished(CompilationResult)          │ Workspace / profiles
┌──────────────▼──────────────┐  ┌──────────────────────┐  ┌───▼───────────────┐
│ DockerCompilerManager       │  │ AssemblyParser       │  │ ProjectController │
│  images · probes · ownership│  │ AssemblyDiffProcessor│  │ ProfileStore      │
│ JobManager (scheduler)      │  │ AssemblyTextProcessor│  │ Workspace (JSON)  │
└──────────────┬──────────────┘  └──────────────────────┘  └───────────────────┘
┌──────────────▼──────────────┐
│ IExecutionBackend           │   DockerExecutionBackend  ·  MockExecutionBackend
│ ILanguageStrategy × 7       │   CompilerRegistry (+ ToolchainProbe)
└─────────────────────────────┘
```

| Module | Responsibility | Page |
| --- | --- | --- |
| `JobManager` | bounded queue, per-pane replacement, cancellation, shutdown, stale-result filtering | [Job Manager](job-manager.md) |
| `DockerExecutionBackend` | one container per job, limits, cleanup, artifacts | [Docker Backend](docker-backend.md) |
| `DockerCompilerManager` | async image listing/pull/delete, probes, ownership | [Compiler Registry](compiler-registry.md) |
| `CompilerRegistry` | metadata, verification state | same |
| strategies | exact compiler commands, output parsing | [Language Strategies](language-strategies.md) |
| `AssemblyParser` | structure and CFG | [Assembly Parser](assembly-parser.md) |
| diff engine | minimal diffs, patches | [Diff Engine](diff-engine.md) |
| `Workspace`, `ProjectController`, `ProfileStore` | projects and profiles | [Workspaces](workspaces.md) |
| `Theme` | tokens, roles | [Theme System](theme-system.md) |

## Lifecycle of one compilation (detailed)

1. **Edit.** `CodeEditor::textChanged` → `MainWindow::onInputTextChanged`: `++m_inputRevision`, restart the
   debounce timer (default 300 ms).
2. **Request.** Timer or **F5** → `compileAllPanes()`. For each pane: build a `CompilationRequest` (pane id,
   revision, source, image, language, syntax, flags, timeout 30 s, target architecture from the verified probe or
   the daemon, extra project files + main file path). The pane shows *Compiling…* and a Cancel button.
3. **Schedule.** `DockerCompilerManager::submitRequest` → `JobManager::submitJob`: assign a job id, supersede the
   pane's older jobs (cancel running ones, replace the waiting one in place), enqueue, fill free slots.
4. **Execute.** A worker runs `DockerExecutionBackend::execute`:
   1. register the job and container name (`octos_<pid>_<job>`);
   2. validate limits (source size), pick the strategy, create two temp dirs (input `0700`, output `0750`);
   3. write the source and the **validated** extra files into the input dir;
   4. `docker run --rm --pull=never …` with the hardening flags, `QProcess` owned by the worker;
   5. poll every ≤ 100 ms: cancel flag, timeout, output into bounded buffers (flood → kill);
   6. on any end: kill the client, **remove the container**, record failures as leaked;
   7. read the output file with a hard size bound (regular file inside the sandbox only);
   8. `strategy->processOutput(...)`, then **re-apply** everything measured here (truncation flags, image,
      command, warnings, duration), classify the failure origin;
   9. build typed artifacts (assembly/bytecode, raw output, diagnostics, metadata) with ids, provenance, checksum.
5. **Hand-off.** The worker appends the result to the manager's inbox and pokes the GUI thread.
6. **Accept or drop.** `handleTaskFinished` frees the slot, starts the next job, and emits `jobFinished` only if
   the job is the pane's newest and its revision is not superseded.
7. **Present.** `MainWindow::onCompilationResultReceived` (guards again), `presentResult()`, version/arch/digest
   from the registry if verified, `pane->setResult`, `showSelectedArtifact`, analysis panel, editor markers.

## Data ownership

| Data | Owner | Lifetime |
| --- | --- | --- |
| `CompilationRequest` | copied into the queue record and the worker | until the job ends |
| `CompilationResult` | created by the worker, moved through the inbox, copied into the pane | until the next result |
| temp directories | the worker (`QTemporaryDir`, RAII) | until `execute()` returns |
| container | the worker (cleanup before return); registry entry removed after cleanup | job duration |
| `AssemblyModule` | `shared_ptr` held by the analysis panel | until the next analysis |
| `Workspace` | `ProjectController` (working + saved copy) | while the project is open |
| settings | `AppSettings` singleton | process |

## Errors and attribution

Every failure has an origin (`Compiler`, `Docker`, `Limits`, `Internal`) so the UI can say whose problem it is
([Error Reference](../reference/errors.md)). Warnings (truncation, failed cleanup) travel in
`CompilationResult::warnings` and are shown even for successful results.

## Cross-cutting rules

See the *ground rules* in the [Development Guide](index.md#ground-rules-used-throughout-the-code).

## Adding a language

See [Extending OCTOS](extending.md).
