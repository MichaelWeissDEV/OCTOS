# Getting Started with OCTOS

Welcome to OCTOS! This guide will help you get started in 5 minutes.

## Prerequisites Checklist

Before starting, ensure you have:

- [ ] Docker installed and running
- [ ] 4GB RAM available
- [ ] 5GB free disk space
- [ ] Linux, macOS, or Windows with WSL2
- [ ] Basic terminal knowledge

### Quick Setup Verification

```bash
# Test Docker
docker run hello-world

# Test Git
git --version

# Test CMake
cmake --version
```

---

## ⚡ 5-Minute Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/MichaelWeissDEV/OCTOS.git
cd OCTOS
```

### 2. Build the Project

```bash
mkdir build && cd build
cmake ..
make -j4
```

### 3. Run OCTOS

```bash
./OCTOS
```

That's it! The application should launch with a dark UI and welcome screen.

---

## 📖 First Steps in the UI

### 1. Create Your First Compilation

1. **Select Language**: Click the dropdown labeled "Select Language"
2. **Choose Language**: Pick **C++** for your first example
3. **Write Code**: In the editor on the left, paste:

```cpp
#include <iostream>
int main() {
    int x = 42;
    std::cout << "Hello: " << x << std::endl;
    return 0;
}
```

4. **Compile**: Click the **"Compile"** button
5. **View Assembly**: See the generated assembly in the right pane!

### 2. Try Another Language

1. Select **Rust** from the dropdown
2. Copy this code:

```rust
fn main() {
    let x = 42;
    println!("Hello: {}", x);
}
```

3. Click **Compile**
4. Observe the assembly output differences

### 3. Use Code Snippets

- Click **"Snippets"** to see pre-built examples
- Double-click any snippet to load it
- Great for learning how different languages compile!

---

## 🎮 Key Features Explained

### The Main Window

```
┌─────────────────────────────────────────────────────┐
│  OCTOS - Assembly Compiler Visualizer              │
├──────────────┬─────────────────────────────────────┤
│  Language    │  Compiler Selection                 │
│  C++         │  gcc:latest                         │
├──────────────┼─────────────────────────────────────┤
│  Code Editor │  Assembly Output Pane               │
│  (Left)      │  (Right)                            │
│              │                                      │
│  [Compile]   │  [Filters...]   [Highlight]        │
└──────────────┴─────────────────────────────────────┘
```

### Compilation Process

1. **Write Code** → Code goes to temporary file
2. **Docker Run** → Compiler runs in isolated container
3. **Generate Assembly** → Compiler outputs `.s` or bytecode
4. **Display** → Assembly shown in right pane with highlighting

### Filters Menu

Customize your assembly view with:
- 👁️ Hide segment directives
- 👁️ Hide data directives
- 👁️ Hide CFI directives
- 👁️ Show metadata labels
- 🔄 Demangle C++ symbols
- And more!

---

## 🎓 Learning Paths

### Path 1: Beginner - "Learn Assembly"

1. Start with simple C programs
2. Compare `int x = 5;` in C vs C++
3. Try loops: `for (int i = 0; i < 10; i++)`
4. Examine function calls
5. Look at memory operations

### Path 2: Intermediate - "Optimization"

1. Write inefficient C++ code
2. Add compiler flags: `-O0`, `-O1`, `-O2`, `-O3`
3. Compare generated assembly
4. Understand optimization techniques
5. Practice writing optimized code

### Path 3: Advanced - "Multi-Language"

1. Implement same algorithm in 6 languages
2. Compare assembly output
3. Analyze performance implications
4. Study calling conventions
5. Explore architecture-specific optimizations

---

## 🛠️ Common Tasks

### Change Compiler Flags

1. Click **Compiler Manager** (or use menu)
2. Find your desired image
3. Flags are applied per-compilation

### Add Custom Snippets

1. Open **Snippet Manager**
2. Click **Add Snippet**
3. Enter code and language
4. Use for repeated testing

### Manage Docker Images

1. Open **Compiler Manager**
2. View available compiler images
3. Download new compilers
4. Remove unused images (saves disk space)

### Export Assembly

1. Right-click in assembly pane
2. Select **Copy All** or **Select All**
3. Paste into editor/file

---

## 🐛 Troubleshooting

### Docker Connection Error

```
Error: Failed to connect to Docker daemon
```

**Solution**: Start Docker
```bash
sudo systemctl start docker
```

### First Compilation Takes Long

**Why**: Docker pulls compiler image (1-2GB)
**Solution**: Be patient! Subsequent compilations are instant due to caching

### No Assembly Output

**Why**: Language/compiler combination not supported yet
**Solution**: See [README.md](../README.md#major-features) for supported languages

### Out of Memory

**Why**: Docker container ran out of RAM
**Solution**: 
```bash
docker system prune
# Restart Docker with more RAM in settings
```

---

## 💡 Pro Tips

1. **Keyboard Shortcuts**
   - `Ctrl+Enter` - Compile
   - `Ctrl+S` - Save
   - `Ctrl+L` - Clear output

2. **Use Recent Files**
   - Quick access to previously compiled files
   - Accessible from File menu

3. **Compare Outputs**
   - Write two different implementations
   - Note assembly differences
   - Understand performance implications

4. **Explore Snippets**
   - Built-in snippets for all languages
   - Great for learning idioms
   - Starting points for experiments

5. **Read Assembly Comments**
   - Assembly has C line number comments
   - Use filters to show/hide metadata
   - Understanding helps with optimization

---

## 🎯 Next Steps

After getting comfortable with OCTOS:

1. **Deep Dive**
   - Read [User Guide](user-guide/index.md)
   - Study compiler internals
   - Experiment with different architectures

2. **Contribute**
   - Add a new language
   - Improve UI/UX
   - Fix bugs
   - See [CONTRIBUTING.md](../CONTRIBUTING.md)

3. **Share**
   - Show colleagues/friends
   - Write blog posts
   - Star the repository
   - Submit pull requests

---

## 📚 Resources

- 📖 [README.md](../README.md) - Full documentation
- 💾 [Installation Guide](installation.md) - Setup help
- 🤝 [Contributing Guide](../CONTRIBUTING.md) - How to help
- 💬 [Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions) - Ask questions

---

## ❓ FAQ

**Q: Is OCTOS free?**
A: Yes! MIT licensed and open source.

**Q: Do I need Docker?**
A: Yes, for compiler isolation and consistency.

**Q: Can I compile on Windows?**
A: Yes, with WSL2. See [Installation Guide](installation.md).

**Q: How can I contribute?**
A: Fork, make changes, submit PR. See [Contributing](../CONTRIBUTING.md).

**Q: What languages does OCTOS support?**
A: C, C++, Rust, Java, Python, C#, Ada. More coming!

---

## 🎉 Congratulations!

You're now ready to explore assembly code like a professional! 

Have fun, and don't hesitate to ask questions in the [Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions) or [Issues](https://github.com/MichaelWeissDEV/OCTOS/issues).

Happy compiling! 🚀
