# Projects

OCTOS has two modes: the classic **single-file** mode and a **project** mode with several files.

## Single-file mode

One editor, no tab bar. File ▸ New/Open/Save work on one file. Nothing changes for existing users.

## Creating and opening projects

* **Project ▸ New Project…** asks for a name and the main source file (a relative path such as `main.cpp` or
  `src/main.cpp`), creates the project and opens the *Project* dock. Nothing is written until you save.
* **Project ▸ Open Project…** opens a `.octos` file. The file is **validated before** your current session is
  touched; if it is invalid you get a message and keep working.
* **Project ▸ Save Project** (Ctrl+Alt+S), **Save Project As…**, **Close Project**.

## The Project dock

A tree of the files. `●` marks unsaved files, *italic* marks headers, **bold** marks the main file of the
active target. Double-click opens a tab. Right-click offers *Open*, *Use as main file of the target*,
*Remove from project* and *Add file…*. **+ File** adds a file (path relative to the project, e.g.
`src/util.h`; headers are recognised by extension: `.h .hpp .hh .hxx .inc .ads`). A project always keeps at
least one file.

## Tabs and saving

Each file has its own tab (with find/replace). A dot in the tab title means the file has unsaved changes.

| Action | Effect |
| --- | --- |
| **Ctrl+S** | saves **only the current file** into the project file. Other unsaved files stay unsaved — the project file on disk is rewritten with the *saved* state plus this file, atomically |
| **Ctrl+Alt+S** | saves the whole project |
| **File ▸ Save As…** (in project mode) | saves the project under a new name |
| closing a dirty tab | asks: Save / Discard / Cancel |
| closing the window | asks to save a dirty project |

If a save fails (read-only folder, disk full) the previous project file is **untouched**, your changes stay
marked unsaved, and you can retry.

Switching tabs never modifies the project: it only changes which file is compiled and sets the language of the
compile panes. The main file of the target changes only through *Use as main file*.

!!! note "Single-file actions and projects"
    **File ▸ New**, **File ▸ Open**, drag & drop and recent files work on stand-alone files. While a project is
    open they first ask whether to close the project, so they can never overwrite a project file.

## What gets compiled

The file in the **active tab** is the main source. *All other project files* are copied next to it into the
container (read-only), so `#include "util.h"` works — also for headers in sub-folders: for C/C++ OCTOS adds
`-I/input` and the main file's directory. Only the main file is compiled into assembly (one translation unit,
one listing); other `.cpp` files can be `#include`d but are not built as separate objects. Limit: 1 MB of
source in total.

## Project files on disk

`octos-project-v1` JSON, see [Project File Format](../reference/project-file-format.md). Highlights:

* validated when loaded: duplicate names/ids, targets pointing to missing files, unsafe paths (absolute, `..`,
  drive letters, backslashes, control characters) and files from a **newer** OCTOS are refused with a message;
* written atomically;
* every path is project-relative and checked again when it is copied into a container.

## Profiles and targets

A project carries its own profiles (shown with *(project)*) and build targets. The active target is the one
whose main file is used; there is currently no dialog to choose between several targets — use *Use as main
file* (see [Limitations](../limitations.md)).
