# Compilers & Languages

OCTOS runs each toolchain in its own Docker container. A *language strategy* turns a request into the exact
compiler invocation and turns the output back into a structured result
([Language Strategies](../development/language-strategies.md)).

## Support matrix

| Language | Level | Artifact | Images | Output file / source |
| --- | --- | --- | --- | --- |
| C / C++ (GCC) | Supported | assembly | `gcc:<tag>` | `/output/output.s` ← `/input/source.c(pp)` |
| C / C++ (Clang) | Supported | assembly | `silkeh/clang:<tag>`, any image whose name contains `clang` | same |
| Rust | Supported | assembly | `rust:<tag>` | `/output/output.s` ← `/input/source.rs` |
| Java | Supported | JVM bytecode | `eclipse-temurin:<tag>`, `openjdk:<tag>` | stdout of `javap`, classes in `/output` |
| Python | Supported | CPython bytecode (`dis`) | `python:<tag>` | stdout; `/output/source.pyc` |
| C# | Experimental | CIL | `mono:<tag>` | stdout of `monodis` |
| Ada | Experimental | assembly | `gnat:<tag>` / `gcc` with GNAT | `/output/output.s` |

The registry lists default images (`gcc:latest`, `silkeh/clang:latest`, `rust:latest`,
`eclipse-temurin:latest`, `python:latest`, `mono:latest`, `gnat:latest`) and infers metadata for any other tag of
a known repository. Unknown images are *recognised* only, never *verified*.

**Platform note.** Docker pulls images for your CPU. On Apple Silicon and ARM Linux the assembly is
**AArch64**; on x86 hosts it is x86-64. x86-only options are passed only for x86 targets.

## C and C++ (GCC and Clang)

Invocation (inside the container):

```
g++|gcc|clang++|clang  -S  -fno-asynchronous-unwind-tables  [-masm=intel|att]  [-g]  <your flags>
                       [-I/input -I/input/<dir of main file>]  /input/source.cpp  -o /output/output.s
```

* `-S` stops after code generation; `-fno-asynchronous-unwind-tables` removes most `.cfi_*` noise.
* `-masm=` only for x86 targets (GCC rejects it on aarch64; Clang would only warn).
* `-g` is added unless you pass any `-g…` flag; it produces the `.loc` lines used for source mapping.
* The `-I` flags appear only in [project mode](../user-guide/projects.md) when extra files exist.
* Clang vs GCC is chosen by the image name (`clang` anywhere in it → Clang).
* Diagnostics: `file:line:col: error|warning|note: message`.

Flags that OCTOS removes: output-related (`-o`, `-c`, `--output…`), anything with shell metacharacters.
`-O0 … -O3`, `-Os`, `-Ofast`, `-march=…`, `-std=…`, `-fno-…`, `-W…` pass through.

## Rust

```
rustc  --crate-type=<lib|bin>  --emit=asm  --crate-name octos_snippet  --edition=2021
       -C codegen-units=1  [-C opt-level=N]  [-C llvm-args=--x86-asm-syntax=intel|att]  [-g]
       <other flags>  /input/source.rs  -o /output/output.s
```

* **Crate type is explicit.** *Auto* (the default) builds a **binary if the source declares `fn main(`** and a
  **library otherwise**, so `pub fn add_two(x: i32) -> i32 { x + 2 }` compiles. A `--crate-type` in your flags
  wins; profiles can force *Binary* or *Library*.
* Your flags are folded into **one value per setting**: a single `--emit=asm` (yours is dropped), one
  `--crate-type`, one `--edition` (default 2021), one `-C opt-level` (`-O` is `opt-level=2`; if you give several
  the last wins). `-g` is added unless you pass a `debuginfo` level.
* `codegen-units=1` and a fixed crate name make the output deterministic.
* The x86 syntax option is passed only for x86 targets.
* A flag's separate value never swallows a following flag (`--crate-type -O` is a dangling flag plus `-O`).
* Diagnostics: `error[E0601]: … --> file:line:col`.
* GCC-only flags (`-masm=`, `-S`, `-fPIC`, `-march=`, …) are dropped.

## Java

`javac -d /output -g /input/<Class>.java`, then **every** generated class is disassembled with
`javap -c -v -p` (found with `find … -name '*.class' | sort`): packages, inner classes (`Outer$Inner`) and
secondary top-level classes are all included. The file name follows the first `public` type (or the first type,
or `Main`). Everything is passed as positional parameters to a shell script; nothing is interpolated.
Diagnostics: `File.java:LINE: error: message`.

## Python

The source is `compile()`d first (so a `SyntaxError` yields file, line and column), then written as
`/output/source.pyc`, then `dis.dis` prints the bytecode, headed by a line stating the Python version and
that this is *CPython bytecode, not native machine assembly*. Different Python versions = different images
(`python:3.10`, `python:3.12`) and visibly different bytecode.

## C# (experimental)

`mcs` (or `csc`) builds `Program.exe`, `monodis` prints the CIL. If neither compiler exists in the image the
pane says so explicitly (exit 127) — use **Verify** to see missing tools.

## Ada (experimental)

`gcc -c -S [-masm=…] <flags> source -o /output/output.s` in an image with GNAT.

## Required tools per family (used by *Verify*)

| Family | Required binaries | Version command |
| --- | --- | --- |
| GCC | `gcc`, `g++` | `--version` |
| Clang | `clang`, `clang++` | `--version` |
| rustc | `rustc` | `--version` |
| OpenJDK | `javac`, `javap` | `-version` |
| CPython | `python3` | `--version` |
| Mono | `mcs`, `monodis` | `--version` |

## Reproducibility

Every result stores the **exact command**, image, image ID (when verified), limits and durations in the
*Compiler Metadata* artifact. Rust output is made deterministic (`codegen-units=1`, fixed crate name). Two runs
with the same image, flags and source produce the same listing.
