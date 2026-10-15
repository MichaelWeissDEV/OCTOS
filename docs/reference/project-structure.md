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
│   ├── MainWindow.cpp / .h      # Main GUI window (compile flow, menus, results)
│   ├── MainWindowProject.cpp    # Project mode, editor tabs, profiles, docks, layout
│   ├── core/                    # Core definitions & data models
│   │   ├── AppSettings.cpp / .h       # Configuration & persistence
│   │   ├── CompilationRequest.h       # Structured compile request
│   │   ├── CompilationResult.h        # Structured compile result
│   │   ├── CompilerDefinition.h       # Toolchain metadata
│   │   ├── CompilerEnums.h            # Languages, flavors, job states
│   │   ├── CompilationArtifact.h/.cpp # Typed artifacts (ids, provenance, lifecycle)
│   │   ├── ResultPresentation.h       # Status badge / notice decision (testable)
│   │   ├── ToolchainProbe.h/.cpp      # In-image toolchain verification
│   │   ├── WorkspaceModel.h/.cpp      # Validated, atomically saved project model
│   │   ├── ProfileStore.h/.cpp        # User-level compiler profiles
│   │   └── Diagnostic.h               # Structured compiler errors/warnings
│   ├── parser/                  # AssemblyParser, AssemblyModel (functions, blocks, CFG, metrics)
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
│   │   ├── ProjectController.cpp      # Open project, per-file dirty state, saving
│   │   └── SnippetManager.cpp         # Code snippet library
│   ├── ui/                      # Qt Widgets & dialogs
│   │   ├── CompilerOutputPane.cpp     # Output pane widget
│   │   ├── CompilerManagerDialog.cpp  # Docker compiler manager dialog
│   │   ├── AnalysisPanel.cpp          # Functions / blocks / statistics from AssemblyModel
│   │   ├── DiffDialog.cpp, DiffTableModel.cpp  # Async diff dialog + virtual model
│   │   ├── CodeEditor.cpp             # Editor: markers, indent, line numbers
│   │   ├── SourceEditorTab.cpp        # Editor tab + find/replace bar
│   │   ├── ProjectExplorerDock.cpp    # Project file tree
│   │   ├── ProfileDialog.cpp          # Profile editor / compare picker
│   │   └── Theme.cpp                  # Design tokens + generated stylesheets
│   ├── highlighters/            # Syntax highlighters
│   │   ├── AsmHighlighter.cpp         # Assembly syntax highlighter
│   │   ├── ArtifactHighlighter.cpp    # LLVM IR / JVM / CPython bytecode / CIL
│   │   └── CppHighlighter.cpp         # C++ syntax highlighter (+ Rust, Python)
│   └── utils/
│       └── AssemblyTextProcessor.cpp  # Assembly filters & line mapper
├── tests/                       # Automated test suite
│   ├── CMakeLists.txt           # Test build definitions
│   ├── unit/                    # Unit tests
│   │   ├── test_compiler_strategies.cpp
│   │   ├── test_assembly_processor.cpp
│   │   ├── test_job_manager.cpp
│   │   ├── test_docker_security.cpp
│   │   ├── test_settings_and_files.cpp
│   │   └── … (see docs/development/testing.md for the complete list)
│   ├── integration/             # Docker / host-compiler integration tests
│   ├── fuzz/                    # libFuzzer targets + shared property bodies
│   ├── benchmarks/              # octos_bench (not part of ctest)
│   └── data/asm/                # Real Clang output corpus for the parser
└── CMakeLists.txt               # Main CMake build definition
```
