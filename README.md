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

## Project Statistics

- **Language**: C++ 17
- **Framework**: Qt 6
- **License**: MIT
- **Status**: Work In Progress

## What is OCTOS?

**OCTOS** is a desktop application for studying and optimizing compiled code. Write code in multiple programming languages, compile it to assembly, and examine the generated machine instructions with a beautiful, feature-rich UI.

It uses Docker for isolated compilation and provides a rich UI for visualization.

Originally developed as an internal pet project, OCTOS is now open source!


![Overview](docs/img/1.png)

Multiple Compiler Versions can be used and compared side by side:

![Compared Compiler Versions](docs/img/2.png)

---

**Status**: OCTOS is an open-source project currently under active development. While functional, it is not yet considered production-ready. This project was inspired by tools like [Godbolt](https://godbolt.org/), but is designed specifically for offline usage. OCTOS is a personal learning and development tool, not a competitor to Godbolt. If you find this project useful, contributions and feedback are welcome!

---

## Key Features

- **Multi-language compilation**: C/C++, Rust, Java, Python, C#
- **Docker-based isolated compilation**: Safe, reproducible builds
- **Real-time assembly visualization**: See generated code instantly
- **Advanced code filtering and annotation**: Customize your view
- **Syntax highlighting**: For multiple assembly variants
- **Compiler error display**: Clear error messages
- **Snippet management system**: Save and reuse code

### Multi-Language Support
- **C / C++** (GCC, Clang)
- **Rust** (rustc)
- **Java** (javac)
- **Python** (py_compile)
- **C#** (csc)


---

## System Requirements

### Hardware
- **CPU**: Modern multi-core processor (2+ cores recommended)
- **RAM**: 4GB minimum (8GB+ recommended for Docker)
- **Storage**: 20GB+ for Docker images and project files 

*Note*: Docker images are downloaded separately and are not included in the repository.

### Software
- **Docker**: >= 20.10 (for containerized compilation)
- **Qt Runtime**: >= 6.0
- **C++ 17 compiler**
- **Operating System**: Linux (primary), macOS/Windows with WSL2

*Note*: Windows is not officially supported as development and testing are conducted exclusively on Linux. macOS support is limited due to platform differences. Contributions for Windows and macOS compatibility are welcome.

---

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
- **gcc:latest** for C/C++
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

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines. Don't worry about perfection - this is a personal project but every contribution is welcome!

---

## Learn More

This project is perfect for:
- Computer Science students learning compilers
- Developers optimizing code
- Assembly language enthusiasts
- Performance engineers

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
- [ ] Fix known issues
- [ ] Add comprehensive documentation

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**Note**: This project was converted from an internal repository. All original contributors retain credit for their work. Contributions are welcome to help improve the project.

---

## Community & Support

### Getting Help
- **Documentation**: Check the [docs](docs/) folder (Coming Soon)
- **Report Issues**: Use [GitHub Issues](https://github.com/MichaelWeissDEV/OCTOS/issues)
- **Discussions**: Join [GitHub Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions)

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
- [Installation Guide](INSTALL.md) - Setup instructions
- [Contributing Guide](CONTRIBUTING.md) - How to contribute
- [Issue Tracker](https://github.com/MichaelWeissDEV/OCTOS/issues)
- [Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions)

---

**Status**: OCTOS is actively maintained with regular updates and improvements. Contributions and feature suggestions are welcome.

**Originally created as an internal project · Now open source on GitHub · All contributors welcome**
