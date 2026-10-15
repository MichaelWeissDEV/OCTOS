# User Guide

This guide explains every feature of OCTOS and — where it matters — *why* it behaves the way it does.

## The workflow in one picture

```
 ┌── Source editor ──┐      ┌──────────── Compiler pane A ────────────┐
 │ your code         │ ───▶ │ image · flags · syntax    [Cancel]      │
 │ (tabs, markers)   │      │ badge · notice · Artifact: Assembly ▾   │
 └───────────────────┘      │ assembly / IR / bytecode / diagnostics  │
        │                    └─────────────────────────────────────────┘
        │  every edit        ┌──────────── Compiler pane B ────────────┐
        └──────────────────▶ │ another image / flags …                 │
                             └─────────────────────────────────────────┘
        Diff View ▸ compare A and B          View ▸ Assembly Analysis ▸ functions, blocks, CFG
```

1. You edit the source. After a short pause (default 300 ms) every pane starts a compilation in its own
   container.
2. Each pane shows the **result** of *its* compiler: assembly, LLVM IR, bytecode, structured diagnostics.
3. You compare panes with the **Diff View**, inspect the structure with the **Assembly Analysis** panel, and
   save profiles you want to reuse.

## Pages

| Page | Contents |
| --- | --- |
| [Interface](interface.md) | every menu, button, dock and shortcut |
| [Compiler Manager](compiler-manager.md) | download, verify, delete compiler images |
| [Comparing Compilers](compiler-comparison.md) | multiple panes, closing, what is shared between them |
| [Profiles](profiles.md) | save and reuse compiler configurations; compare two profiles |
| [Assembly View](assembly-view.md) | highlighting modes, source ↔ assembly mapping, opcode tooltips |
| [Filters](filters.md) | what each assembly filter removes and why |
| [Artifacts](artifacts.md) | assembly, IR, bytecode, diagnostics, metadata, saving |
| [Diff Viewer](diff-viewer.md) | options, navigation, incomplete diffs, patch export |
| [Assembly Analysis](analysis.md) | functions, blocks, successors, statistics |
| [Projects](projects.md) | multi-file projects, tabs, explorer, saving |
| [Themes & Layout](themes-and-layout.md) | themes, docks, layout persistence |

## Principles you will notice everywhere

* **Honesty about status.** A badge tells you *who* failed (compiler, Docker, a limit, OCTOS itself). An
  image is only called *Verified* after a real check inside it. Truncated output is labelled *Incomplete*.
* **Nothing is downloaded behind your back.** Compiling never pulls images; deleting an image is always your
  decision.
* **Stale results never show up.** If you keep typing, results of older compilations are discarded.
* **Unknown stays unknown.** The analysis never invents jump targets, sizes or source mappings.
