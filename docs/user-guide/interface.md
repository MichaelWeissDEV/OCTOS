# Interface

## Window overview

```
┌ File  Filters  Tools  View  Project  Profiles ─────────────────────────────────────────────┐
│ ┌─ Project ─┐ ┌ Source Code ────────[Highlight][+ Compare][Diff View][Close Others]┐ ┌ pane ┐ │
│ │ tree      │ │ ┌ tabs: main.cpp ● │ util.h ┐                                      │ │      │ │
│ │ (project  │ │ │ editor with line numbers, │                                      │ │      │ │
│ │  mode)    │ │ │ error markers, find bar   │                                      │ │      │ │
│ └───────────┘ │ └───────────────────────────┘                                      │ └──────┘ │
│               └────────────────────────────────────────────────────────────────────┘          │
│ ┌ Assembly Analysis (dock, bottom) ───────────────────────────────────────────────────────┐   │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

All areas are separated by splitters and the docks can be moved, floated, tabbed or closed.
The layout is stored when you quit ([Themes & Layout](themes-and-layout.md)).

## Source editor

| Feature | Behaviour |
| --- | --- |
| Line numbers | left gutter, width adapts to the line count |
| Current line | highlighted (theme colour) in editable editors |
| Syntax highlighting | C/C++-style for C, C++, Java, C#, Ada; Rust and Python have their own highlighters. Colours follow the theme. |
| Auto-pairing | `(`, `[`, `{`, `"`, `'` insert the closing character. Quotes after a letter or backslash are not paired (apostrophes in comments). |
| Indentation | **Tab** inserts spaces up to the next 4-column stop; **Shift+Tab** removes one level; with a selection both act on all selected lines. **Enter** keeps the indentation of the current line and adds one level after `{` or `:`. |
| Error markers | a dot in the gutter and a wavy underline at the position of each compiler diagnostic; hover the gutter dot for the message |
| Find / replace | see below |
| Drag & drop | drop a file onto the window to load it (asks to close an open project first) |

### Find and replace

Open the bar with the platform's standard **Find** (Ctrl+F) and **Replace** (Ctrl+H on most platforms)
shortcuts. **Enter** / **Shift+Enter** (or ▼ ▲) jump to the next / previous match and wrap around; **F3**
and **Shift+F3** continue without the bar having focus. The bar shows the number of matches
(*Match case* toggles case sensitivity). **Replace** replaces the current match, **Replace all** all
matches in one undo step. Read-only editors (the compiler panes) can be searched but never modified.
**Esc** closes the bar.

## Compiler panes

Every pane is one independent *compilation configuration*.

| Control | Meaning |
| --- | --- |
| Title | `language \| image \| syntax \| flags`, plus a status badge |
| **Language** | C++, C, Rust, Java, Python, C#, Ada. Changing it refreshes the compiler list. |
| **Syntax** | Intel / AT&T for x86 targets; disabled for languages without the choice |
| **Compiler** | installed Docker images compatible with the language |
| **Flags** | free-form compiler flags, e.g. `-O3 -march=native` (see [Flags](#flags)) |
| **Artifact** | appears when the result has more than one view ([Artifacts](artifacts.md)) |
| **Save…** | saves the shown artifact to a file |
| **Cancel** | visible while a job is queued or running |
| Info line | compiler image (and version if verified), runtime, output size; tooltip = exact command |
| Notice | warnings such as truncated output or a container that could not be removed |
| ✕ | closes the pane and cancels its jobs |

### Status badges

| Badge | Meaning |
| --- | --- |
| `Compiling...` | job queued/running |
| `Success` | compiled; nothing was cut off |
| `Incomplete` | compiled, but output was truncated — see the notice |
| `Compiler error` | the toolchain ran and reported errors (see Diagnostics) |
| `Docker error` | daemon/image problem: not running, image missing, container command not found |
| `Limit exceeded` | timeout, output flood, size limit |
| `OCTOS internal error` | OCTOS could not set the job up (temp directories, no strategy …) |
| `Timeout` | exceeded the time limit (default 30 s) |
| `Cancelled` | you or a newer edit stopped it |

### Flags

Flags are split like a shell would (quotes allowed) but are **never** executed by a shell. Output-related
flags (`-o`, `-c`, `--emit` …), shell metacharacters (`; | & $ ` > < ( ) { }`) and flags that do not make
sense for the language (`-masm=` for Rust, …) are dropped. See
[Language Strategies](../development/language-strategies.md).

## Menus

### File
`New` (Ctrl+N), `Open…` (Ctrl+O), `Recent Files`, `Save` (Ctrl+S), `Save As…`, `Exit`. While a *project* is
open, these act on the project: Save saves the current file into the project, Save As saves the project under
a new name, and New/Open ask to close the project first.

### Filters
Assembly display filters, see [Filters](filters.md).

### Tools
`Docker Compiler Manager…`, `Insert Snippet`, `Theme`, `Auto-Compile on change`.

### View
Toggle the *Project* and *Assembly Analysis* docks; **Reset Layout**.

### Project
`New Project…`, `Open Project…`, `Save Project` (Ctrl+Alt+S), `Save Project As…`, `Add File…`,
`Active Target ▸`, `Active Profile ▸`, `Manage Project Profiles…`, `Close Project` — see [Projects](projects.md).

### Profiles
`Manage Profiles…`, `Save Pane as Profile…`, `Apply Profile to Pane`, `Compare Two Profiles…` — see
[Profiles](profiles.md).

## Source toolbar

| Button | Action |
| --- | --- |
| **Highlight: Off/All/Selection** | cycles the [highlighting mode](assembly-view.md#highlighting-modes) |
| **+ Compare** | adds a compiler pane |
| **Diff View** | opens the [diff viewer](diff-viewer.md) (needs ≥ 2 panes) |
| **Close Others** | closes all panes except the first |

## Keyboard shortcuts

See [Keyboard Shortcuts](../reference/keyboard-shortcuts.md).
