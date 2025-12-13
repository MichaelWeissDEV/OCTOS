# OCTOS - Offline Compiler Tool Optimized Assembly Scrutinizer

<div align="center">
  <img src="assets/OCTOS.png" alt="OCTOS Logo" width="200" height="200">

  **A multi-language assembly compiler and optimizer with Docker containerization**

  [![WIP](https://img.shields.io/badge/Status-Work%20In%20Progress-orange)](https://github.com/MichaelWeissDEV/OCTOS)
  [![License](https://img.shields.io/badge/License-MIT-blue)](#license)
  [![C++](https://img.shields.io/badge/Language-C%2B%2B-blue)](https://en.wikipedia.org/wiki/C%2B%2B)
  [![Qt6](https://img.shields.io/badge/Framework-Qt%206-green)](https://www.qt.io/)
  [![Docker](https://img.shields.io/badge/Containerization-Docker-blue)](https://www.docker.com/)

</div>

---

## Overview

**OCTOS** is an offline compiler tool designed for developers who want to understand, analyze, and optimize compiled code. Write code in multiple programming languages, compile it to assembly, and examine the generated machine instructions with a beautiful, feature-rich UI.

Originally developed as an internal pet project, OCTOS is now open source!


![Overview](docs/img/1.png)

Multiple Compiler Versions can be used and compared side by side:

![Compared Compiler Versions](docs/img/2.png)
---

**IMPORTANT:** THIS IS AN INTERNAL PROJECT NOW OPEN SOURCE. IT IS STILL A WORK IN PROGRESS AND NOT PRODUCTION READY.
THIS WAS INSPIRED BY TOOLS LIKE GODbolt BUT AIMED AT OFFLINE USAGE. 
THER PROJECT IS AWSOME AND YOU SHOULD DEFINITELY CHECK IT OUT: https://godbolt.org/ OCTOS IS NOT A COMPETITOR TO GODBOLT JUST A TOY PROJECT FOR ME AND IF YOU IF YOU WANT. 
I HIGHLY RECOMMEND TO CHECK OUT GODBOLT AND MAYBE  LEAVE A STAR OR SUPORT THE PROJECT IF YOU LIKE IT. THIS IS JUST A PERSONAL PROJECT OF MINE BUT YOU ARE WELCOME TO USE IT AND CONTRIBUTE.



---

## Features

### Multi-Language Support
- **C / C++** (GCC, Clang)
- **Rust** (rustc)
- **Java** (javac)
- **Python** (py_compile)
- **C#** (csc)
- **Ada** (GNAT) (EXPERIMENTAL/WIP)


---

## System Requirements

### Hardware
- **CPU**: Modern multi-core processor (2+ cores recommended)
- **RAM**: 4GB minimum (8GB+ recommended for Docker)
- **Storage**: 20GB+ for Docker images and project files 
**NOTE:** Docker images are downloaded BY YOU they are not included in the repository.

### Software
- **Docker**: >= 20.10 (for containerized compilation)
- **Qt Runtime**: >= 6.0
- **Operating System**: Linux (primary), macOS/Windows with WSL2

*NOTE* : Windows is NOT officially supported since i am only developing and testing on Linux. For Apple the same applies but at least it is POSIX compliant and i could test some basic functionality but i am only working on Linux. You are welcome to contribute fixes for other OSes.


## Quick Start

### Installation

#### Prerequisites
```bash
# Install Docker
sudo apt-get install docker.io

# Ensure Docker daemon is running
sudo systemctl start docker
sudo usermod -aG docker $USER
```

#### Building from Source
```bash
# Clone the repository
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j4

# Run OCTOS
./OCTOS
```

---

## Usage Guide

### Basic Workflow

1. **Select Language**: Choose your target programming language from the dropdown
2. **Write Code**: Use the built-in editor or load a snippet
3. **Compile**: Click "Compile" or press the keyboard shortcut
4. **View Assembly**: See the generated machine code in the output pane
5. **Analyze**: Use filters and tools to understand the code

### Docker Image Management

OCTOS automatically pulls and caches compiler Docker images:
- **gcc:latest** for C/C++/Ada
- **rust:latest** for Rust
- **eclipse-temurin:latest** for Java
- **python:latest** for Python
- **mono** for C#

Manage images in the **Compiler Manager** dialog:
- View available compilers
- Download new compiler images
- Remove unused images

---

## Development

### Project Structure
```
OCTOS/
├── src/
│   ├── main.cpp                 # Application entry point
│   ├── MainWindow.cpp/h         # Main UI window
│   ├── backend/
│   │   ├── DockerCompilerManager.cpp/h
│   │   ├── CompilerDriver.cpp/h
│   │   └── AsyncCompiler.cpp/h
│   ├── ui/
│   │   ├── CompilerOutputPane.cpp/h
│   │   ├── CompilerManagerDialog.cpp/h
│   │   └── Theme.cpp/h
│   ├── managers/
│   │   ├── SnippetManager.cpp/h
│   │   ├── RecentFilesManager.cpp/h
│   │   └── PackageManager.cpp/h
│   ├── strategies/
│   │   └── ILanguageStrategy.h  # Language-specific implementations
│   └── utils/
│       ├── AssemblyTextProcessor.cpp/h
│       └── CompilerDetector.cpp/h
├── assets/
│   └── OCTOS.png                # Application logo
├── CMakeLists.txt
└── README.md
```

### Build Configuration

#### CMake Variables
```bash
# Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimization
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### Build Commands
```bash
# Build with parallelization
make -j$(nproc)

# Clean build
make clean
cmake ..
make

# Run with debugging
./OCTOS --debug
```

### Contributing

I welcome contributions! Whether you want to:
- Add support for new programming languages
- Improve the UI/UX
- Optimize Docker compilation
- Fix bugs
- Improve documentation

**How to contribute:**
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Commit with clear messages (`git commit -m 'Add amazing feature'`)
5. Push to your branch (`git push origin feature/amazing-feature`)
6. Open a Pull Request

---

## Known Limitations

- **Work In Progress**: Some features are still under development
- Requires Docker daemon to be running
- First compilation download may take time (Docker image caching)
- Linux-first support (macOS/Windows require WSL2)
- Assembly output may vary by compiler version

---

## Roadmap

- [ ] Architecture-specific assembly visualization
- [ ] Performance profiling tools
- [ ] Additional language support
- [ ] Assembly diff comparison
- [ ] Enhanced UI/UX features
- [ ] FIX ADA assembly output
- [ ] FIX Known Issues
- [ ] ADD more documentation or anny for the Start :)

---

##  License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**Note:** This is an open source project converted from an internal repository. All original developers retain credit for their contributions. I welcome every contribution to make this project better! But i am not responsible for any issues arising from its use.


---

## Community & Support

### Getting Help
-  **Documentation**: Check the [docs](docs/) folder (COMING SOON)
-  **Report Issues**: Use [GitHub Issues](https://github.com/MichaelWeissDEV/OCTOS/issues)
-  **Discussions**: Join [GitHub Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions)

### Acknowledgments
- **Qt Framework** for the beautiful cross-platform UI
- **Docker** for containerization and compilation isolation
- **Community Contributors** - thank you for making this project better!

---


<div align="center">
  If you find OCTOS useful, please consider starring the repository!
</div>

---

## 🔗 Quick Links

- [GitHub Repository](https://github.com/MichaelWeissDEV/OCTOS)
- [Issue Tracker](https://github.com/MichaelWeissDEV/OCTOS/issues)
- [Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions)

---

**Status**: This project is actively being developed. I am working on new features and improvements regularly!
Feel free to contribute or suggest features!
I am happy about every star and every contribution!
**IMPORTANT:** THIS IS AN INTERNAL PROJECT NOW OPEN SOURCE. IT IS STILL A WORK IN PROGRESS AND NOT PRODUCTION READY.
