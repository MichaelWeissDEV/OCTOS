# Project Structure

The OCTOS source tree is structured into decoupled modules:

```
OCTOS/
├── .github/
│   └── workflows/
│       └── ci.yml               # GitHub Actions CI workflow (Linux & macOS)
├── docs/                        # MkDocs documentation
├── src/
│   ├── main.cpp                 # Application entry point
│   ├── MainWindow.cpp / .h      # Main GUI window
│   ├── core/                    # Core definitions & data models
│   │   ├── AppSettings.cpp / .h       # Configuration & persistence
│   │   ├── CompilationRequest.h       # Structured compile request
│   │   ├── CompilationResult.h        # Structured compile result
│   │   ├── CompilerDefinition.h       # Toolchain metadata
│   │   ├── CompilerEnums.h            # Languages, flavors, job states
│   │   └── Diagnostic.h               # Structured compiler errors/warnings
│   ├── strategies/              # Language-specific compilation strategies
│   │   ├── ILanguageStrategy.h        # Base strategy interface
│   │   ├── LanguageStrategyRegistry.h # Strategy factory & discovery
│   │   ├── StrategyUtils.cpp / .h     # Sanitization & flag utilities
│   │   ├── GccStrategy.cpp / .h       # GCC C / C++
│   │   ├── ClangStrategy.cpp / .h     # Clang C / C++
│   │   ├── RustStrategy.cpp / .h      # Rust toolchain
│   │   ├── JavaStrategy.cpp / .h      # Java javac + javap
│   │   ├── PythonStrategy.cpp / .h    # Python syntax & dis bytecode
│   │   ├── CSharpStrategy.cpp / .h    # Mono C# bytecode
│   │   └── AdaStrategy.cpp / .h       # GNAT Ada pipeline
│   ├── backend/                 # Execution backends & job coordination
│   │   ├── IExecutionBackend.h        # Execution interface
│   │   ├── DockerExecutionBackend.h   # Docker runner with security limits
│   │   ├── DockerExecutionConfig.h    # Security sandbox parameters
│   │   ├── MockExecutionBackend.h     # Mock runner for automated testing
│   │   ├── JobManager.cpp / .h        # Concurrency & revision management
│   │   └── DockerCompilerManager.cpp  # Docker image manager
│   ├── managers/                # Application managers
│   │   ├── RecentFilesManager.cpp     # Recent files tracking
│   │   └── SnippetManager.cpp         # Code snippet library
│   ├── ui/                      # Qt Widgets & dialogs
│   │   ├── CompilerOutputPane.cpp     # Output pane widget
│   │   ├── CompilerManagerDialog.cpp  # Docker compiler manager dialog
│   │   └── Theme.cpp                  # UI themes & styles
│   ├── highlighters/            # Syntax highlighters
│   │   ├── AsmHighlighter.cpp         # Assembly syntax highlighter
│   │   └── CppHighlighter.cpp         # C++ syntax highlighter
│   └── utils/
│       └── AssemblyTextProcessor.cpp  # Assembly filters & line mapper
├── tests/                       # Automated test suite
│   ├── CMakeLists.txt           # Test build definitions
│   ├── unit/                    # Unit tests
│   │   ├── test_compiler_strategies.cpp
│   │   ├── test_assembly_processor.cpp
│   │   ├── test_job_manager.cpp
│   │   ├── test_docker_security.cpp
│   │   └── test_settings_and_files.cpp
│   └── integration/             # Integration tests
│       └── test_docker_integration.cpp
└── CMakeLists.txt               # Main CMake build definition
```
