# Language Strategies

In OCTOS, each programming language has a corresponding strategy implementing the `ILanguageStrategy` interface.

## Interface Definition

```cpp
class ILanguageStrategy {
public:
    virtual ~ILanguageStrategy() = default;

    virtual Language language() const = 0;
    virtual QString languageId() const = 0;
    virtual QString defaultCompilerBinary(const QString& dockerImage) const = 0;
    virtual QString sourceFileName(const QString& sourceCode) const = 0;
    virtual QString outputFileName() const = 0;

    virtual QStringList buildCompilerArgs(const CompilationRequest& req,
                                          const QString& inputFilePath,
                                          const QString& outputDirPath) const = 0;

    virtual bool supportsSyntaxFlavor(SyntaxFlavor flavor) const = 0;
    virtual QStringList sanitizeUserFlags(const QStringList& rawFlags) const = 0;

    virtual CompilationResult processOutput(const CompilationRequest& req,
                                            int exitCode,
                                            const QString& stdoutStr,
                                            const QString& stderrStr,
                                            const QString& outputFileContent) const = 0;

    virtual QVector<Diagnostic> parseDiagnostics(const QString& compilerOutput) const = 0;
};
```

## Strategy Implementations

- **`GccStrategy`**: Supports C and C++. Injects `-S`, `-fno-asynchronous-unwind-tables`, `-masm=intel|att`, `-g`, and writes assembly to `/output/output.s`.
- **`ClangStrategy`**: Supports C and C++ using Clang toolchains. In project mode (`extraFiles` present) GCC and Clang add `-I/input` and the main file's directory.
- **`RustStrategy`**: Compiles Rust with `rustc --emit=asm`. The **crate type is explicit** (`RustCrateType`: `Auto`/`Binary`/`Library`, on `CompilationRequest`): `Auto` picks a binary if the source declares `fn main(` and a **library otherwise**, so a snippet such as `pub fn add_two(x: i32) -> i32 { x + 2 }` compiles. A `--crate-type` given by the user wins. User flags are folded into one value per setting — a single `--emit=asm`, one `--crate-type`, one `--edition` (default 2021) and one `-C opt-level` (`-O` and `-C opt-level=N` collapse, last wins) — so no duplicate or contradictory arguments reach `rustc`. Also: `--crate-name octos_snippet`, `-C codegen-units=1` (deterministic), `-g` unless a debuginfo level is given, Intel/AT&T via `llvm-args` (x86 targets), GCC-only flags are dropped, diagnostics are parsed. A flag's separate value never swallows a following flag (`--crate-type -O`).
- **`JavaStrategy`**: Detects public class names, invokes parameterized `javac`, then disassembles **every** generated class file (`find … -name '*.class' | sort`, `javap -c -v -p`) — packages (`demo/pkg/Outer.class`), inner classes (`Outer$Inner`) and secondary top-level classes are included. (The earlier script only looked at top-level class files and printed nothing for a packaged class.)
- **`PythonStrategy`**: First `compile()`s the source (so a `SyntaxError` yields file/line/column that are parsed into diagnostics — `py_compile` alone raised a different exception type that the diagnostic parser missed), then writes `/output/source.pyc` and prints the `dis` output headed by the Python version. The artifact type is *CPython bytecode*, not native assembly.
- **`CSharpStrategy`**: Disassembles Mono assemblies using `monodis`.
- **`AdaStrategy`**: Pipeline for GNAT compiler toolchains.
