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
- **`ClangStrategy`**: Supports C and C++ using Clang toolchains.
- **`RustStrategy`**: Compiles Rust with `rustc --emit=asm`, handles LLVM assembly syntax flavors, filters out incompatible GCC flags, and extracts Rust diagnostics.
- **`JavaStrategy`**: Detects public class names, invokes parameterized `javac` followed by `javap -c -v -p`, and presents disassembled bytecode.
- **`PythonStrategy`**: Compiles source code to `/output/source.pyc` and generates disassembled CPython bytecode via `dis`.
- **`CSharpStrategy`**: Disassembles Mono assemblies using `monodis`.
- **`AdaStrategy`**: Pipeline for GNAT compiler toolchains.
