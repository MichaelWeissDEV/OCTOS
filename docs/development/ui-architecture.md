# UI Architecture

All widget code lives in `octos_ui`. `MainWindow` is the coordinator; it owns no domain logic.

## Class overview

| Class | File | Role |
| --- | --- | --- |
| `MainWindow` | `MainWindow.cpp/.h` | menus, compile flow (`compileAllPanes`, `onCompilationResultReceived`, `showSelectedArtifact`), panes, theme switching, filters |
| *(same class)* | `MainWindowProject.cpp` | project mode, editor tabs, profiles, docks, layout save/restore |
| `CodeEditor` | `ui/CodeEditor.*` | `QPlainTextEdit` with line-number gutter, markers (`EditorMarker`), indentation, theme colours |
| `SourceEditorTab`, `FindReplaceBar` | `ui/SourceEditorTab.*` | one tab = editor + find/replace bar |
| `CompilerOutputPane` | `ui/CompilerOutputPane.*` | one compilation configuration + its result viewer |
| `AnalysisPanel` | `ui/AnalysisPanel.*` | functions/blocks/statistics from `AssemblyModel` |
| `DiffDialog`, `DiffTableModel`, `DiffFilterProxy` | `ui/Diff*.{h,cpp}` | async diff viewer with a virtual model |
| `ProjectExplorerDock` | `ui/ProjectExplorerDock.*` | file tree of the open project |
| `ProfileDialog`, `CompareProfilesDialog` | `ui/ProfileDialog.*` | profile editor and picker |
| `CompilerManagerDialog` | `ui/CompilerManagerDialog.*` | images, verification, deletion |
| `Theme` | `ui/Theme.*` | design tokens, generated stylesheets, roles |
| highlighters | `highlighters/*` | `AsmHighlighter`, `CppHighlighter`, `RustHighlighter`, `PythonHighlighter`, `ArtifactHighlighter` |

## Compile flow

```
edit / pane change ──▶ onInputTextChanged / onPaneSettingsChanged
                        ++m_inputRevision; debounce timer (Editor/DebounceIntervalMs)
timer / F5 ──────────▶ compileAllPanes()
                        for each pane: CompilationRequest (+ project extra files, target architecture)
                        pane->setBusy(true); DockerCompilerManager::submitRequest → JobManager::submitJob
JobManager::jobFinished ─▶ DockerCompilerManager::compilationResultReceived
                        ▶ MainWindow::onCompilationResultReceived
                           guards: pane exists, job id == pane's current job, revision ≥ m_inputRevision
                           presentResult() → badge + notice
                           fill version/arch/digest from the registry if verified
                           pane->setResult() → selector + diagnostics table
                           showSelectedArtifact() → text/highlighter per artifact type
                           AnalysisPanel::setPaneAssembly() for assembly artifacts
                           updateEditorMarkers()
```

The three guards make stale results harmless even if the scheduler had a bug: the scheduler drops them too.

## Panes and artifacts

`CompilerOutputPane::setResult()` stores the result and fills the artifact combo box (artifacts, minus the
textual Diagnostics artifact when a structured list exists, plus "Diagnostics (n)"). `MainWindow::showSelectedArtifact`
decides what to display:

* no artifacts and no diagnostics → raw error text;
* diagnostics selected → the table;
* `Assembly` → `AssemblyTextProcessor` (filters, demangling, line maps) → `showArtifactText(…, Assembly)`;
* everything else → shown as is with `ArtifactHighlighter` for IR/bytecode/CIL.

`showArtifactText` swaps highlighters: the assembly `AsmHighlighter` is only attached for assembly.

## Editor tabs and the active editor

`m_inputEditor` is always the editor of the *current* tab. `setActiveEditor()` disconnects the old editor's
signals (`textChanged → onInputTextChanged`, modification, cursor) and connects the new one's. A *permanent*
per-editor connection copies edits into the `ProjectController` regardless of which tab is active.
`m_tabPath` maps editors to project paths (empty in single-file mode); `m_highlighters` holds one highlighter
per editor.

Single-file mode is simply "one tab, no project": the tab bar hides itself (`setAutoHide(true)`).

## Docks and layout

`setupDocks()` creates the *Project* dock (hidden until a project opens) and the *Assembly Analysis* dock
(hidden by default) and a *View* menu with their toggle actions. `saveLayout()` stores geometry, dock state
(`saveState(1)`), and the two splitters under `Layout/*`; `restoreLayout()` reads them at the end of the
constructor and never hides the explorer of an open project. The factory state is saved once as
`Layout/defaultState` for *Reset Layout*.

## Signals between components

| Signal | From → To |
| --- | --- |
| `cancelRequested` | pane → `MainWindow` → `DockerCompilerManager::cancelJob` |
| `artifactSelectionChanged` | pane → `showSelectedArtifact` |
| `diagnosticActivated(file, line, col)` | pane → source editor `goToLine` |
| `assemblyLineActivated(paneId, rawLine)` | analysis panel → pane `displayLineForRaw` → editor |
| `sourceLineActivated(line)` | analysis panel → source editor |
| `openFileRequested`, `addFileRequested`, `removeFileRequested`, `setMainRequested` | explorer → `MainWindow` |
| `fileDirtyChanged`, `dirtyChanged`, `projectChanged` | `ProjectController` → tabs, explorer, window title |
| `installedImagesRefreshed` | Docker manager → `MainWindow::refreshDockerCompilerList`, Compiler Manager |

## Testing the UI

GUI tests construct the real `MainWindow` on the `offscreen` platform and drive it through public slots
(`QMetaObject::invokeMethod`), accessors (`activeEditor()`, `projectController()`, `analysisPanel()` …), signals
and screenshots. See `test_main_window.cpp` and `test_project_ui.cpp`. Modal dialogs are answered by a timer that
clicks a button of `QApplication::activeModalWidget()`.

## Conventions

* No hard-coded colours or per-widget stylesheets: use a role (`Theme::setRole`) or a token.
* No blocking calls on the GUI thread.
* A widget that reads data from another thread receives it through a signal or a `QFutureWatcher`.
