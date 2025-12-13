# OCTOS - Offline Compiler Tool Optimized Assembly Scrutinizer


## Project Statistics

- **Language**: C++ 17
- **Framework**: Qt 6
- **License**: MIT
- **Status**: Work In Progress

## What is OCTOS?

OCTOS is a desktop application for studying and optimizing compiled code. Write in 6+ languages, compile to assembly, and analyze the generated machine instructions.
It uses Docker for isolated compilation and provides a rich UI for visualization.

**IMPORTANT:** THIS IS AN INTERNAL PROJECT NOW OPEN SOURCE. IT IS STILL A WORK IN PROGRESS AND NOT PRODUCTION READY.
THIS WAS INSPIRED BY TOOLS LIKE GODbolt BUT AIMED AT OFFLINE USAGE. 
THER PROJECT IS AWSOME AND YOU SHOULD DEFINITELY CHECK IT OUT: https://godbolt.org/ OCTOS IS NOT A COMPETITOR TO GODBOLT JUST A TOY PROJECT FOR ME AND IF YOU IF YOU WANT. 
I HIGHLY RECOMMEND TO CHECK OUT GODBOLT AND MAYBE  LEAVE A STAR OR SUPORT THE PROJECT IF YOU LIKE IT. THIS IS JUST A PERSONAL PROJECT OF MINE BUT YOU ARE WELCOME TO USE IT AND CONTRIBUTE.

## Key Features

Multi-language compilation (C++, Rust, Java, Python, Ada, C#)
Docker-based isolated compilation
Real-time assembly visualization
Advanced code filtering and annotation
Syntax highlighting for multiple assembly variants
Compiler error display
Snippet management system

## Quick Start

```bash
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS
mkdir build && cd build
cmake .. && make -j4
./OCTOS
```

## Requirements

- Docker (for compilation isolation)
- Qt 6 (for UI)
- C++ 17 compiler
- Linux/macOS/Windows (with WSL2)

## Links

- [README](README.md) - Full documentation
- [Installation Guide](INSTALL.md) - Setup instructions
- [Contributing Guide](CONTRIBUTING.md) - How to contribute
- [Issues](https://github.com/MichaelWeissDEV/OCTOS/issues) - Report bugs
- [Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions) - Ask questions

## Learn More

This project is perfect for:
- Computer Science students learning compilers
- Developers optimizing code
- Assembly language enthusiasts
- Performance engineers

## Contributing

I welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. If something is not perfect or missing, feel free to open an issue or submit a pull request. Dont worry about perfection it is a personal project of mine but every contribution is welcome!

---

**Originally created as an internal project · Now open source on GitHub · Everyone is welcome!**
