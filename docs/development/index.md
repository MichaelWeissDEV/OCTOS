# Development Guide

This guide is for developers looking to contribute to OCTOS or understand its internals.

## Architecture

OCTOS is built with C++17 and Qt 6. 
- **Frontend**: The UI is composed of custom Qt widgets (`CodeEditor`, `CompilerOutputPane`, `MainWindow`).
- **Backend**: Compilation is handled asynchronously via `DockerCompilerManager`, which launches and manages Docker containers.
- **Language Strategies**: The `src/strategies/` directory contains `ILanguageStrategy` implementations for different languages, responsible for providing the correct docker command line arguments for compilation or disassembly.

## Adding Language Support

To add support for a new language:
1. Create a new strategy class inheriting from `ILanguageStrategy`.
2. Implement the `getCompilerCommand` method to return the proper Docker invocation.
3. Register the strategy in `CompilerDriver`.
4. Update the UI enums and documentation to reflect the new language.

See the [Contributing Guide](../../CONTRIBUTING.md) for more details on submitting patches.
