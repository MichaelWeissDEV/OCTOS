# Comparing Compilers

Compare different compilers, versions, optimisation levels or languages side by side.

## Adding and closing panes

* **+ Compare** (source toolbar) adds a compiler pane. The new pane starts with the language default and
  compiles immediately.
* **✕** on a pane header closes it. Closing a pane cancels all queued and running jobs of that pane and
  discards any result that is still on its way.
* **Close Others** keeps only the first pane.
* The first pane cannot be closed while it is the only one.

## What is shared and what is not

| Shared by all panes | Per pane |
| --- | --- |
| the source text (the active editor tab) | language, syntax flavour, compiler image, flags |
| the revision counter (stale-result protection) | status badge, notice, artifacts, diagnostics |
| display filters (Filters menu) | the displayed artifact |

Because the source is shared, changing the *language* of a pane makes sense only if the source can be read by
both compilers; OCTOS does not translate code.

## How compilations are scheduled

Each edit increments a revision. After the debounce delay (default 300 ms, *Settings*) every pane submits a
job. The scheduler runs at most *N* jobs at a time (default: number of CPU threads, at least 2); with five
panes and `N = 2` the others wait. A pane never has more than one *waiting* job: a newer request replaces the
older one. A running job of the same pane is cancelled when a newer one arrives. Details:
[Job Manager](../development/job-manager.md).

You can turn off *Auto-Compile on change* in the Tools menu. Then nothing is compiled while you type; press
**Compile (F5)** (source toolbar, or **Tools ▸ Compile Now**, F5 / Ctrl+Return) to compile all panes. Repeated
manual compiles of an unchanged document are fine: a new job replaces the previous one of the same pane.

## Typical comparisons

| Goal | Setup |
| --- | --- |
| optimisation effect | two panes, same image, flags `-O0` vs `-O3` — or use [Profiles](profiles.md) |
| GCC vs Clang | two panes with `gcc:…` and `silkeh/clang:…`, same flags |
| two compiler versions | `gcc:12` vs `gcc:13` |
| C++ vs Rust | the *same algorithm* in two languages cannot share one editor — use two projects/windows |

## Looking at the differences

Open the [Diff Viewer](diff-viewer.md) (**Diff View**) to compare two panes line by line. It warns you if you
compare different artifact kinds (e.g. assembly against bytecode), because such a comparison is not
semantically meaningful.
