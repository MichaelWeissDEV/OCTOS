# Installation Guide

Complete setup instructions for OCTOS on different operating systems.

## Prerequisites

- **Docker**: Required for compilation isolation
- **Qt 6**: For the GUI framework
- **C++ Compiler**: GCC, Clang, or MSVC
- **CMake**: Build system

---

##  Linux (Ubuntu/Debian)

### Install Dependencies

```bash
# Update package lists
sudo apt-get update

# Install build tools
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    qt6-tools-dev \
    libgl1-mesa-dev

# Install Docker
sudo apt-get install -y docker.io

# Add user to docker group (optional, to avoid sudo)
sudo usermod -aG docker $USER

# Start Docker daemon
sudo systemctl start docker
sudo systemctl enable docker
```

### Build OCTOS

```bash
# Clone the repository
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS

# Create build directory
mkdir build && cd build

# Configure build
cmake ..

# Compile (use -j for parallel jobs)
make -j$(nproc)


```

### Run OCTOS

```bash
# From build directory
./OCTOS

# Or if installed system-wide
OCTOS
```

---

##  macOS (NOTE: Not Officially Tested)

### Using Homebrew

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake qt@6 docker

# Install Docker Desktop (GUI app with CLI)
brew install --cask docker
```

### Build OCTOS

```bash
# Clone the repository
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS

# Create build directory
mkdir build && cd build

# Configure build (specify Qt path)
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" ..

# Compile
make -j$(sysctl -n hw.ncpu)

# Run
./OCTOS
```

### Docker Desktop Setup

1. Download Docker Desktop from [docker.com](https://www.docker.com/products/docker-desktop)
2. Install and start the application
3. Grant permissions when prompted

##  Windows (NOTE: Not Officially Tested and WSL2 Recommended)


