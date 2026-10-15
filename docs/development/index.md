# Development Guide

For contributors and anyone who wants to understand OCTOS' internals. The code base is C++17 and Qt 6
(Widgets), built with CMake; the tests use Qt Test and run headless.

## Map

| Topic | Page |
| --- | --- |
| Big picture: modules, data flow, lifecycle of one compilation | [Architecture](architecture.md) |
| Which thread does what, and how they hand over data | [Threading Model](threading.md) |
| Building, build options, IDEs, sanitizers, benchmarks | [Building & Running](building.md) |
| Scheduler, bounded queue, shutdown | [Job Manager](job-manager.md) |
| Containers: flags, limits, cleanup, threat model | [Docker Backend](docker-backend.md) |
| Image verification, probes, ownership | [Compiler Registry](compiler-registry.md) |
| How each language is invoked | [Language Strategies](language-strategies.md) |
| Assembly → functions, blocks, CFG | [Assembly Parser](assembly-parser.md) |
| Diff algorithm, normalisation, patches | [Diff Engine](diff-engine.md) |
| Projects, profiles, artifacts | [Workspaces & Artifacts](workspaces.md) |
| Widgets, MainWindow, docks, panes | [UI Architecture](ui-architecture.md) |
| Design tokens, roles, contrast | [Theme System](theme-system.md) |
| Tests, sanitizers, fuzzing, benchmarks | [Testing & Quality](testing.md) |
| Add a language / architecture / artifact type / theme | [Extending OCTOS](extending.md) |
| *Why* things are the way they are | [Design Decisions](design-decisions.md) |
| CI | [Continuous Integration](ci.md) |

## Source tree at a glance

```
src/core/        data model, registry, probe, workspace, profiles, settings, result presentation
src/parser/      AssemblyParser + AssemblyModel (functions, blocks, CFG, metrics)
src/strategies/  one ILanguageStrategy per language
src/backend/     JobManager, Docker backend/config/manager, mock backend
src/managers/    ProjectController, recent files, snippets
src/utils/       diff engine, assembly text filters, opcode reference, TSan annotations
src/ui/          widgets (see UI Architecture)
src/highlighters syntax highlighters (theme-aware)
tests/           unit/ integration/ fuzz/ benchmarks/ data/
```

Libraries: `octos_core` (no widgets), `octos_ui` (all widgets), the `OCTOS` executable (only `main.cpp`).
Tests link the same libraries, so what is tested is what ships.

## Ground rules used throughout the code

1. **No blocking Docker calls on the GUI thread.** Everything goes through `QProcess` signals or a worker.
2. **No lock is held while calling out** (backend, signals, Docker).
3. **Cancellation is cooperative and bounded**: workers poll a flag at least every 100 ms.
4. **Untrusted input is validated twice**: when it enters (project load, flags) and where it is used
   (copying files into a container).
5. **Incomplete is never presented as complete** (diffs, artifacts, analysis).
6. **Unknown stays unknown** (operands, jump targets, sizes, mappings).
7. **Tests must be able to fail**: no `|| true`, no assertions that only check that a mock recorded a call.
