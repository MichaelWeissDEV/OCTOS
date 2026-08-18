<div align="center">
  <img src="assets/OCTOS.png" alt="OCTOS Logo" width="200" height="200">

  # OCTOS
  **Offline Compiler Explorer & Assembly Analyzer**

  [![Status](https://img.shields.io/badge/Status-Alpha%20%2F%20WIP-orange)](#project-status)
  [![Documentation Status](https://readthedocs.org/projects/octos/badge/?version=latest)](https://octos.readthedocs.io/en/latest/?badge=latest)
  [![License](https://img.shields.io/badge/License-MIT-blue)](#license)
  [![C++](https://img.shields.io/badge/Language-C%2B%2B17-blue)](https://en.wikipedia.org/wiki/C%2B%2B)
  [![Qt6](https://img.shields.io/badge/Framework-Qt%206-green)](https://www.qt.io/)
  [![Docker](https://img.shields.io/badge/Backend-Docker-blue)](https://www.docker.com/)

  ![OCTOS Main Window](assets/screenshots/main-window.png)
</div>

---

## Overview

**OCTOS** is a desktop application for studying and optimizing compiled code. Write code in multiple programming languages, compile it locally, and examine the generated machine instructions or bytecode through a feature-rich interface.

By utilizing Docker for isolated compilation environments, OCTOS ensures safe, reproducible builds without requiring complex toolchain configurations on your host system.

## Major Features

- **Local Compilation**: OCTOS performs compilation locally and does not submit source code to a remote compilation service. Network access may be required to download compiler container images. Once required images are available locally, compilation can operate without a remote compilation backend.
- **Docker-based Backends**: Isolated compiler toolchains using Docker containers, eliminating complex host configurations.
- **Side-by-Side Comparison**: Compare output from different compiler versions or optimization flags.
- **Assembly Visualization**: Advanced syntax highlighting, filtering, and annotation for multiple architectures.
- **Multi-Language Support**: Support for a variety of languages, including native, bytecode, and IL compilation targets.

## Language & Compiler Support

| Language | Status | Compilation Target | Supported Compilers |
| :--- | :--- | :--- | :--- |
| **C / C++** | Supported | Native Assembly | GCC, Clang |
| **Rust** | Supported | Native Assembly | rustc |
| **Java** | Supported | Bytecode (javap) | javac |
| **C#** | Supported/Partial | IL (Mono) | csc, mcs |
| **Python** | Partial | Syntax Checking | py_compile |
| **Ada** | Planned | — | — |

## Quick Start

### Prerequisites
- **Docker** installed and running
- **CMake** (3.16+) and **Qt 6**
- **C++17** compatible compiler

### Build and Run
```bash
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS
mkdir build && cd build
cmake ..
make -j$(nproc)
./OCTOS
```
*Note: The first compilation for any language may take a few moments as the required Docker image is pulled.*

## Documentation

**[📚 Read the official OCTOS Documentation here](https://octos.readthedocs.io/)** *(or visit the [project dashboard](https://app.readthedocs.org/projects/octos/))*

The complete documentation is built with MkDocs and hosted on Read the Docs. It includes:
- Complete Installation & Quick Start Guides
- User Guide for interface and features
- Compiler and Language support matrix
- Architecture and Development instructions

## Project Status & Limitations

**Status:** Alpha / Work in Progress

OCTOS is under active development. Current known limitations include:
- Windows support relies on WSL2; macOS support is currently limited.
- Docker is strictly required for the compilation backend.
- Assembly extraction relies on specific Docker image toolchains and may vary by image version.

See the [Roadmap](ROADMAP.md) for planned improvements.

## Contributing

Contributions are welcome! Please see our [Contributing Guide](CONTRIBUTING.md) for details on how to set up your environment, follow our coding standards, and submit pull requests.

## Acknowledgments

OCTOS is heavily inspired by the incredible work done on [Compiler Explorer (Godbolt)](https://godbolt.org/). While OCTOS is designed as a distinct offline desktop tool, we are grateful for the inspiration provided by the Compiler Explorer project.

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.
