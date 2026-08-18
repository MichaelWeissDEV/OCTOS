# Limitations

OCTOS is in an Alpha state and has a number of known limitations:

- **Platform Support**: Development and testing are primarily conducted on Linux. Windows requires WSL2, and macOS support may require additional manual Docker configuration.
- **Ada Support**: The Ada language backend is currently a placeholder and does not correctly generate assembly.
- **Container Dependency**: OCTOS strictly requires a running Docker daemon. Native host compilation is intentionally unsupported to guarantee isolation and consistency.
- **Initial Compile Delay**: The first time you select a compiler, OCTOS must pull the corresponding Docker image. This can take several minutes depending on your internet connection.
