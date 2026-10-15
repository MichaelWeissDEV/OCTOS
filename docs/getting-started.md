# Getting Started

A guided first session: install a compiler, compile a snippet, compare two optimisation levels, look at the
structure of the output.

## Prerequisites

* Docker (Docker Desktop on macOS/Windows, the Docker service on Linux), running.
* A built OCTOS ([Installation](installation.md)).

## 1. Start OCTOS

```bash
./OCTOS
```

The first launch shows one source editor on the left and one empty compiler pane on the right. If the pane's
**Compiler** list is empty and the badge says *No Image*, no compiler image is installed yet.

## 2. Install a compiler

1. **Tools ▸ Docker Compiler Manager…**
2. The banner at the top should be green (*Docker Connected*). If it is red, start Docker.
3. Select the category **C++**, pick a tag (e.g. `gcc:13`) and click **Download**; confirm. The progress bar
   shows the pull. Images are a few hundred MB to a few GB.
4. When the download finishes, click **Verify** next to the image. The *Compatibility* column should turn
   into **Verified – gcc (GCC) 13…** — that proves the tools really are in the image. Close the dialog.

OCTOS never downloads anything on its own: compiling with a missing image gives a *Docker error*, not a
surprise download.

## 3. Your first compilation

Select **C++** as language, and the installed image as compiler. Type:

```cpp
int square(int x) { return x * x; }
```

After a short pause the pane shows assembly. If you turned auto-compile off, press **F5**.

* The title shows what was used; the info line shows the image, the compile time and the output size.
* Hover a mnemonic (e.g. `imul`) for an instruction description.
* Type a syntax error and watch the badge turn to **Compiler error**, the *Artifact* selector offer
  **Diagnostics (n)**, and a dot/underline appear in the editor gutter. Double-click a diagnostic to jump to
  the line.

## 4. Compare optimisation levels

1. Click **+ Compare**. A second pane appears with the same defaults.
2. Put `-O0` in the flags of pane 1 and `-O3` in pane 2.
3. Click **Diff View**. Rows tinted red exist only in pane 1, green only in pane 2. **Next Change →** walks
   through the differences; **Show only differences** hides the rest.

To avoid retyping flags, save them as [profiles](user-guide/profiles.md) and use
**Profiles ▸ Compare Two Profiles…**.

## 5. Look at the structure

Open **View ▸ Assembly Analysis**. It lists the functions of the output with their block, instruction and call
counts; select one to see its basic blocks and how they connect (`Taken`, `Fallthrough`, tail calls, unknown
computed jumps). Double-click a block's *Source lines* cell to jump into your code.

## 6. Several files

**Project ▸ New Project…** gives you a project with tabs and a file tree; headers in the project can be
`#include`d. See [Projects](user-guide/projects.md).

## Next steps

* [Interface](user-guide/interface.md) — every control explained
* [Artifacts](user-guide/artifacts.md), [Diff Viewer](user-guide/diff-viewer.md),
  [Assembly Analysis](user-guide/analysis.md)
* [Languages & Compilers](compilers/index.md)
* [Troubleshooting](troubleshooting.md)
