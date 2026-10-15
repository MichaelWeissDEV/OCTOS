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
```

---

##  macOS (developed and tested on Apple Silicon, macOS 26, Qt 6.11, Apple clang 21)

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




---

## After building: verify the installation

```bash
# 1. the tests (headless)
cd build && ctest --output-on-failure

# 2. Docker is reachable
docker info --format '{{.ServerVersion}} {{.Architecture}}'
```

`Architecture` tells you which assembly you will see: `x86_64` → x86-64, `aarch64` → AArch64 (Apple Silicon,
ARM Linux).

Then start `./OCTOS`, open **Tools ▸ Docker Compiler Manager…**, check the banner is green, download one image
(e.g. `gcc:13`), click **Verify**, and follow [Getting Started](getting-started.md).

## Build options

| Option | Default | Meaning |
| --- | --- | --- |
| `-DBUILD_TESTING=OFF` | ON | skip tests (and the need for Qt Test) |
| `-DOCTOS_BUILD_BENCHMARKS=OFF` | ON | skip `octos_bench` |
| `-DOCTOS_BUILD_FUZZERS=ON` | OFF | libFuzzer targets (needs a fuzzer-capable clang) |
| `-DCMAKE_BUILD_TYPE=Release` | — | optimised build; run Debug **and** Release before contributing |

More: [Building & Running](development/building.md).

## Tested configurations

| Platform | Compiler | Qt | Status |
| --- | --- | --- | --- |
| macOS 26 / Apple Silicon | Apple clang 21 | 6.11 | full suite (Debug, Release, ASan+UBSan, TSan) |
| Ubuntu 22.04 | GCC (CI) | 6.2 | built and tested by CI; not verified locally by the maintainer of this change |
| Windows | — | — | not tested; WSL2 recommended |

## Docker notes

* **macOS/Windows**: Docker Desktop must be *running*. Containers live in a virtual disk with a size limit;
  "no space left on device" means that disk is full ([Troubleshooting](troubleshooting.md)).
* **Linux**: add your user to the `docker` group or run OCTOS as a user that may use the socket.
* GUI applications on macOS may not inherit the shell `PATH`; `docker` must be found in the environment the
  application starts with (`/usr/local/bin` or `/opt/homebrew/bin`).

## Uninstalling and cleaning up

* Delete the build directory / the installed binary.
* Settings: see [Settings](reference/settings.md) for the location (or delete the store).
* Docker images that OCTOS downloaded stay until you delete them in the Compiler Manager (or `docker rmi`).
* Leftover containers (only after a crash) are named `octos_<pid>_<job>`: `docker ps -a --filter name=octos_`.
