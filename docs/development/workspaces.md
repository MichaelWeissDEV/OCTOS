# Workspaces, Profiles & Artifacts

## Project file (`Workspace`, format `octos-project-v1`)

JSON with `format_version`, `name`, `source_files` (relative `path`, `content`, `is_header`, `language`),
`profiles`, `targets` and `configuration`.

**Loading is validating.** `fromJson()` rejects (with a message) or reports:

* oversized files (256 MB), wrong types, invalid JSON;
* **format gate:** a missing version is accepted with a warning, an unrecognised one is refused, a
  *newer* one (`octos-project-v2`) is refused with "created by a newer OCTOS". Older formats would be
  migrated step by step (the hook exists; format 1 is the oldest, so there is nothing to migrate yet);
* duplicate file paths / profile ids / target names (no longer silently merged);
* dangling references: target → file, active profile/target, include paths;
* unsafe paths: absolute, `..` escapes, drive letters, `~`, backslashes, control characters, non-normalised;
* invalid environment variable names, too many/too large files.

Unknown languages produce warnings (`Workspace::warnings()`), missing optional fields get defaults.

**Saving is atomic.** `saveToFile()` validates first (an invalid workspace is never written) and writes
through `QSaveFile`; a failure leaves the previous project file untouched (tested with a read-only
directory).

## Paths

`normalizeRelativePath()` / `isSafeRelativePath()` define what a project path may be.
`resolveInside(root, rel)` returns the absolute location *verified to stay inside the root*, also against
symlinks (existing components must not be symlinks leaving the root). `materializeTo()` and the
Docker backend (extra project files) use it; a project can therefore not write outside its sandbox.

## `ProjectController`

Owns the open project: *working* copy (what the editors show) and *saved* copy (what is on disk).

* per-file **dirty state** (content differs from the saved copy, or the file is not on disk yet);
* `saveFile(path)` writes the saved copy **plus only that file's new content** (atomically), so other
  unsaved files are never half-written; `saveProject()`/`saveProjectAs()` write everything;
* a failed save keeps everything dirty; removing a file cleans targets (no dangling references);
* active profile/target, `ensureTargetForMain()`.

`MainWindow` only wires it to widgets (`MainWindowProject.cpp`): editor tabs (one per file, tab bar hidden
for the single-file case), `ProjectExplorerDock`, find/replace, Project/Profiles menus.

## Compiling a project

`Workspace::buildRequest(profile, target)` / the window attach every other project file as
`CompilationRequest::extraFiles`; the backend places them next to the main source in `/input`
(validated paths, counted in `maxSourceBytes`). For C/C++ the strategies add `-I/input` and the main
file's directory. **Only the main file is compiled** (one translation unit → one assembly listing);
other `.cpp` files are available to `#include`, not built as separate objects.

## Profiles

`CompilerProfile`: name, image, version, language, optimization, flags, target architecture, output
format, Rust crate type, syntax flavour. `effectiveFlags()` = optimisation + flags.
`ProfileStore` keeps **user-level profiles** in `AppSettings` (survive sessions); a project also carries
its own. *Profiles ▸ Compare Two Profiles* opens two panes, applies both and compiles the same source —
no manual re-entry.

## Artifacts

`CompilationArtifact` carries: stable id (`<job>:<index>:<type>`), type, name, content, size, SHA-256,
source, compiler profile, compilation id (job), creation time, `truncated`, and **separate** location
fields: `containerPath` (informational), `hostTempPath` (transient; **never set by the backend** — the
sandbox directory is deleted when the job ends), `persistentPath` + `storage` (only after an explicit
`saveTo()`, atomic). Types: Assembly, LLVM IR, JVM bytecode, CPython bytecode, CIL, Diagnostics,
CompilerMetadata (JSON: image, exact command, exit code, limits), RawOutput.

The pane's *Artifact* selector lists them; the highlighter follows the type (x86 rules never colour JVM or
Python bytecode); diagnostics are a structured list (double-click jumps to the source line) and appear
as markers in the editor gutter; *Save…* writes the shown artifact.
