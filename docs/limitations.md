# Limitations

As of OCTOS 0.3.0, the following limitations are documented:

- **Docker Daemon Requirement for Live Compilation**:
  Running live compilations requires a running Docker daemon on the host. If Docker is offline, OCTOS informs the user and automated tests gracefully skip container tests while running all unit tests.
- **Language Support Levels**:
  - **Ada**: Marked as **Planned**. Full assembly extraction requires an appropriate container image with GNAT configured.
  - **C#**: Marked as **Experimental**. Bytecode / IL extraction requires a Mono image with `mcs` and `monodis`.
- **First Compilation Delay**:
  Pulling a new compiler image from Docker Hub may take several minutes depending on network bandwidth. Once cached locally, compilation is fast and operates offline.
- **Multi-File Projects**:
  Version 0.3.0 supports single-file compilation units and self-contained scripts. Full multi-file build system support (CMake/Cargo inside containers) is planned for future versions.
