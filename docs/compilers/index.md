# Compilers & Languages

OCTOS relies on isolated Docker containers to run independent compiler toolchains locally.

## Support Matrix (v0.3.0)

| Language | Status | Compilation Target | Primary Images | Compiler Invocation |
| :--- | :--- | :--- | :--- | :--- |
| **C / C++** | **Supported** | Native Machine Assembly | `gcc`, `silkeh/clang` | `gcc`/`g++`/`clang`/`clang++` `-S -fno-asynchronous-unwind-tables -masm=[intel\|att] -g -o /output/output.s` |
| **Rust** | **Supported** | Native Machine Assembly | `rust` | `rustc --emit=asm -C llvm-args=--x86-asm-syntax=[intel\|att] -g -o /output/output.s` |
| **Java** | **Supported** | JVM Bytecode | `eclipse-temurin`, `openjdk` | Parameterized compilation: `javac -d /output <Source>.java && javap -c -v -p /output/<Class>.class` |
| **Python** | **Supported** | CPython Bytecode & Syntax | `python` | Safe compilation to `/output/source.pyc` and bytecode disassembly via `python3 -m dis` |
| **C#** | **Experimental** | CIL / Intermediate Language | `mono` | `mcs -out:/output/Program.exe /input/Program.cs && monodis /output/Program.exe` |
| **Ada** | **Planned** | Native Assembly | `gcc` (with GNAT) | Requires GNAT toolchain container |

## Implementation Details

### Native Assembly (C, C++, Rust)
For C, C++, and Rust, OCTOS compiles the input file directly into annotated machine assembly (`/output/output.s`).
- In C/C++, users can select between **Intel** (`-masm=intel`) and **AT&T** (`-masm=att`) syntax flavors.
- In Rust, syntax flavor is handled via LLVM arguments (`-C llvm-args=--x86-asm-syntax=intel` or `att`), without leaking incompatible GCC flags.
- Source line debugging directives (`.loc`) are extracted and used to calculate real-time source-to-assembly line mappings.

### JVM Bytecode (Java)
- The Java strategy parses the source code to identify the declared public class, interface, enum, or record.
- It compiles with `javac -d /output -g` and disassembles with `javap -c -v -p`.
- Invocation is strictly parameterized without shell string concatenation, preventing shell injection vulnerabilities.

### CPython Bytecode (Python)
- The Python strategy safely validates syntax and produces bytecode inside `/output/source.pyc`, ensuring compatibility with read-only `/input` mounts.
- It disassembles the compiled bytecode using the standard Python `dis` module and clearly marks the output as CPython bytecode instructions (not native assembly).

### C# and Ada
- **C#**: Marked as Experimental. It requires a Mono container providing `mcs`/`csc` and `monodis`. If Mono is unavailable in the selected image, a clear explanatory diagnostic is shown.
- **Ada**: Marked as Planned. Requires a container with the GNAT compiler. If GNAT is not found, the user is notified with an explicit status message rather than a generic failure.
