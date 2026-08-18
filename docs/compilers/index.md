# Compilers & Languages

OCTOS relies on Docker images to run isolated compiler toolchains. 

## Support Matrix

| Language | Status | Compilation Target | Primary Images |
| :--- | :--- | :--- | :--- |
| **C / C++** | Supported | Native Assembly | `gcc`, `clang` |
| **Rust** | Supported | Native Assembly | `rust` |
| **Java** | Supported | Bytecode (javap) | `eclipse-temurin`, `openjdk` |
| **C#** | Supported/Partial | IL (Mono) | `mono` |
| **Python** | Partial | Syntax Checking | `python` |
| **Ada** | Planned | — | — |

## Implementation Details

- **Native Assembly**: For C/C++ and Rust, OCTOS injects compiler flags (e.g., `-masm=intel`, `-g`) to generate annotated assembly code.
- **Bytecode / IL**: For languages like Java and C#, OCTOS compiles the source into bytecode or intermediate language, then uses disassembly tools (like `javap` or `monodis`) to present the output.
- **Syntax Checking**: Python execution currently only performs a `py_compile` pass to check syntax, not disassembly.
