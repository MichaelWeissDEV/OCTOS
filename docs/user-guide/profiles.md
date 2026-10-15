# Compiler Profiles

A **profile** stores everything needed to reproduce a compilation setup, so you never have to retype flags:

| Field | Meaning |
| --- | --- |
| Name | shown in menus ("GCC 13 -O3") |
| Compiler image | e.g. `gcc:13` |
| Language | C++, C, Rust, Java, Python, C#, Ada |
| Optimization | e.g. `-O3`, or `-C opt-level=3` for Rust |
| Flags | additional flags, e.g. `-Wall -march=native` |
| Syntax | Intel / AT&T |
| Output | Assembly or LLVM IR (Clang, Rust) |
| Rust crate type | Auto (binary if `main` exists, otherwise library), Binary, Library |
| Target architecture | informational note (e.g. `x86_64`) |

The flags handed to the compiler are *optimisation first, then flags*.

## Where profiles live

* **User profiles** — kept in your settings and available in every window and project. Edited with
  **Profiles ▸ Manage Profiles…**.
* **Project profiles** — stored inside a `.octos` project file. They appear in the same menus with the
  suffix *(project)*.

Profile IDs are derived from the name and made unique (`gcc-o0`, `gcc-o0-2`, …). Corrupt stored profiles are
skipped with a warning instead of breaking the application.

## Typical workflow: compare `-O0` and `-O3`

1. **Profiles ▸ Manage Profiles…** → **New**, name it *GCC -O0*, choose image `gcc:13`, optimisation `-O0`.
   **Duplicate** it, rename the copy *GCC -O3* and change the optimisation. OK.
2. **Profiles ▸ Compare Two Profiles…** → pick A and B. OCTOS creates two panes if needed, applies both
   profiles and compiles the same source in both.
3. Open the **Diff View** to see exactly what the optimiser changed.

## Other actions

* **Save Pane as Profile…** — captures a pane's current language, image, flags and syntax under a name you
  choose. (Flags are stored as typed, so an `-O3` you typed in the flags box is stored in *Flags*.)
* **Apply Profile to Pane** — loads a profile into a pane (asks which pane when there are several). If the
  profile's image is not installed you get a message pointing to the Compiler Manager; the other settings
  are still applied.

## Compiling a project with a profile

`Workspace::buildRequest` (used by the project machinery) combines the profile with the target's main file
and sends all other project files along as additional files, see [Projects](projects.md).
