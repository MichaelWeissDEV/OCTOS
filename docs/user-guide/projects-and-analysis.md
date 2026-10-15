# Projects, Profiles & Assembly Analysis

## Single file vs. project

OCTOS starts in the familiar single-file mode (one editor, no tab bar). **Project ▸ New Project…** creates a
project with several files; **Open Project…** opens a `.octos` file. In a project:

* the **Project** dock lists the files (folders, `●` = unsaved, *italic* = header, **bold** = main file of the
  active target); double-click opens a tab; right-click to add/remove or *use as main file*;
* each file has its own tab with find/replace (**Find** / **Replace**: the platform's standard shortcuts, normally Ctrl+F and Ctrl+H), auto-indent and Tab/Shift+Tab;
* **Ctrl+S** saves the current file into the project file, **Ctrl+Alt+S** saves the whole project (File ▸ *Save As…*, normally Ctrl+Shift+S, saves the project under a new name while a project is open);
* **File ▸ New / Open**, drag & drop and recent files work on *stand-alone* files: while a project is open they first ask to close it, so they can never overwrite a project file. Switching tabs never changes the project; use *Use as main file* to change the target's main file;
* the open tab is the file that is compiled; all other project files (headers!) are available to
  `#include` inside the container. Only the main file is turned into assembly.

Project files are validated when loaded: duplicate names, paths that try to leave the project and files
from a newer OCTOS are refused with a message.

## Compiler profiles

A profile stores image, language, optimisation, flags and syntax. **Profiles ▸ Manage Profiles…** edits
your profiles (kept between sessions); **Save Pane as Profile…** captures a pane; **Apply Profile to Pane**
loads one; **Compare Two Profiles…** opens two panes with e.g. *GCC -O0* and *GCC -O3* for the same
source.

## Reading a compiler pane

* The badge tells *who* failed: **Compiler error**, **Docker error**, **Limit exceeded**, **OCTOS internal
  error**; **Incomplete** means the output was truncated (see the notice).
* *Artifact* selects what you see: assembly, LLVM IR, JVM / CPython bytecode, CIL, structured
  **Diagnostics** (double-click → jump to the line), compile metadata (the exact command), raw output.
* **Cancel** stops a running compilation; **Save…** writes the shown artifact to a file.
* The info line shows compiler (and the version reported by the image *if it was verified*), runtime and
  output size.

## Assembly analysis (View ▸ Assembly Analysis)

Built from one structured parse of the listing (x86-64 GNU assembler output):

* **Functions** with block, instruction, call and branch counts; double-click jumps to the function in
  the assembly output (lines hidden by your filters are reported, not guessed);
* **Basic blocks** of the selected function with their successors (`Taken`, `Fallthrough`,
  `Unconditional`, and `IndirectUnknown` for computed jumps — OCTOS never invents a target) and their
  source lines (double-click the *Source lines* column to jump into your code);
* **Statistics**: instruction frequencies and totals. Counts describe the listing, **not** speed.

## Layout

All areas are dockable/resizable; **View** toggles the Project and Analysis docks; the layout is saved on
exit and **View ▸ Reset Layout** restores the default. Colours follow the theme in every widget.
