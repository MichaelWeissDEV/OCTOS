# Contributing to OCTOS

Thank you for your interest in contributing to OCTOS!  I am excited to work with you.

## Code of Conduct

- Be respectful and inclusive
- Welcome all levels of experience
- Focus on ideas, not people
- Help others learn and grow

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork**: `git clone https://github.com/YOUR-USERNAME/OCTOS.git`
3. **Create a branch**: `git checkout -b feature/your-feature-name`
4. **Make your changes** and test thoroughly
5. **Commit with clear messages**: `git commit -m "Add feature description"`
6. **Push to your fork**: `git push origin feature/your-feature-name`
7. **Open a Pull Request** with a clear description

## Development Setup

```bash
# Install dependencies
sudo apt-get install cmake qt6-base-dev docker.io

# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4

# Run tests (if available)
make test
```

## Code Style

- Follow the existing code formatting (use `.clang-format`)
- Use meaningful variable names
- Add comments for complex logic
- Keep functions focused and small

### Formatting

Run clang-format before committing:
```bash
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## Commit Messages

- Use clear, descriptive messages
- Reference issues where applicable: `Fixes #123`
- Use imperative mood: "Add feature" not "Added feature"

Example:
```
Add Rust assembly output formatting

- Implement objdump parsing for Rust
- Add syntax highlighting support
- Fixes #45
```

## Pull Request Process

1. Update the README.md with any new features
2. Test your changes thoroughly
3. Ensure all compiler backends still work
4. Reference any related issues
5. Wait for review feedback


## Reporting Issues

When reporting a bug:
1. Check if the issue already exists
2. Provide a minimal reproducible example
3. Include your environment details (OS, Docker version, etc.)
4. Share error messages and logs
5. Describe expected vs actual behavior

## Questions?

- Ask in [GitHub Discussions](https://github.com/MichaelWeissDEV/OCTOS/discussions)
- Open an issue with the `question` label
- Check existing documentation and issues

## Recognition

All contributors will be acknowledged in the project.

Thank you for making OCTOS better!
