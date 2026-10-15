# OCTOS Documentation

**OCTOS** is an offline *Compiler Explorer*: you type source code, OCTOS compiles it with the compilers of
your choice inside throw-away Docker containers, and shows you the generated assembly, LLVM IR or bytecode
next to your code — for several compilers, versions and optimisation levels at once. It then helps you
*understand* that output: functions, basic blocks, control flow, diffs between two compilations.

Everything runs locally. No source code leaves your machine, and after the images are downloaded no network
is needed.

!!! note "Development state"
    The code base is at version string `0.4.1` plus unreleased development work (see
    [`CHANGELOG.md`](https://github.com/MichaelWeissDEV/OCTOS/blob/main/CHANGELOG.md)). This documentation
    describes what the code in the repository does *today*, including its limits
    ([Limitations](limitations.md)).

## Where to start

| I want to… | Read |
| --- | --- |
| install and build OCTOS | [Installation](installation.md) |
| compile my first snippet | [Getting Started](getting-started.md) |
| understand every part of the window | [Interface](user-guide/interface.md) |
| install, verify and delete compilers | [Compiler Manager](user-guide/compiler-manager.md) |
| compare `-O0` and `-O3` (or GCC and Clang) | [Comparing Compilers](user-guide/compiler-comparison.md), [Profiles](user-guide/profiles.md) |
| read the output and its filters | [Assembly View](user-guide/assembly-view.md), [Filters](user-guide/filters.md), [Artifacts](user-guide/artifacts.md) |
| see *what changed* between two outputs | [Diff Viewer](user-guide/diff-viewer.md) |
| analyse functions, blocks, control flow | [Assembly Analysis](user-guide/analysis.md) |
| work with several source files | [Projects](user-guide/projects.md) |
| change colours, layout, limits | [Themes & Layout](user-guide/themes-and-layout.md), [Settings](reference/settings.md) |
| something does not work | [Troubleshooting](troubleshooting.md), [Error Reference](reference/errors.md), [FAQ](faq.md) |
| understand how it works / contribute | [Development](development/index.md) |

## Documentation map

* **Getting Started** — installation, first compilation.
* **User Guide** — one page per feature, with the reasoning behind the behaviour you see.
* **Languages & Compilers** — what each language produces and which exact compiler command is used.
* **Development** — architecture, threading, Docker backend, parser, diff engine, themes, testing, how to
  extend OCTOS.
* **Reference** — settings keys, environment variables, project file format, data model, error messages,
  keyboard shortcuts, glossary.
