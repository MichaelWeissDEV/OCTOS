# Getting Started

Welcome to OCTOS! This guide will help you get started with your first compilation.

## Prerequisites

Before starting, ensure you have:

- Docker installed and running
- A built version of OCTOS (see [Installation](installation.md))

## 1. Start OCTOS

Run the application from your build directory:

```bash
./OCTOS
```

## 2. Your First Compilation

1. **Select Language**: Click the language dropdown and select **C++**.
2. **Select Compiler**: Ensure a compiler like `gcc:latest` is selected.
3. **Write Code**: Enter the following example in the source editor:

```cpp
#include <iostream>

int main() {
    int x = 42;
    std::cout << "Hello: " << x << std::endl;
    return 0;
}
```

4. **Compile**: Click the **Compile** button. The first compilation might take a few moments as Docker pulls the necessary compiler image.
5. **View Assembly**: Once compilation finishes, the generated assembly will appear in the right-hand pane.

## 3. Compare Compilers

1. Click **Add Compiler** to open a new compiler pane next to the existing one.
2. Select a different compiler version (e.g., `clang:latest`).
3. Click **Compile** again to see side-by-side assembly outputs.

## Next Steps

Explore the detailed documentation:

- [User Guide - Interface](user-guide/interface.md)
- [User Guide - Filters](user-guide/filters.md)
- [Supported Languages](compilers/index.md)
