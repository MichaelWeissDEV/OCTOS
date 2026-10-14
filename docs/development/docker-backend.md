# Docker Execution Backend & Security Sandbox

OCTOS executes all compiler toolchains within temporary, isolated Docker containers. This document details the security constraints and runtime management of these containers.

## Security Constraints

Compilation jobs are executed with aggressive sandboxing defaults:

- **Network Isolation (`--network none`)**:
  Containers have no network access. Compilation cannot phone home, fetch external code, or access the local host network. (Network access is strictly isolated to Docker image pull operations in `DockerCompilerManager`).
- **Resource Constraints**:
  - Memory limit: Default `512m` (configurable via `AppSettings::dockerMemoryLimit()`).
  - CPU allocation: Default `1.5` cores (configurable via `AppSettings::dockerCpuLimit()`).
  - PID limit: Default `128` (prevents fork bombs).
- **Privilege Separation**:
  - `--security-opt=no-new-privileges`: Prevents setuid binaries inside the container from escalating privileges.
  - `--cap-drop=ALL`: Drops all kernel capabilities by default.
- **Filesystem Isolation**:
  - Input directory is mounted strictly read-only: `-v <host_temp_in>:/input:ro`.
  - Output directory is mounted into a dedicated volume: `-v <host_temp_out>:/output:rw`.
  - Working directory is locked to `/output`.
- **Output Size Caps**:
  - Maximum output size is capped at 10 MB to prevent unbounded memory growth from malformed compiler dumps.

## Container Lifecycle & Cleanup

Containers are assigned a unique deterministic name:
```
octos_<PID>_<JobID>
```

When a compilation completes, is cancelled, or reaches the timeout threshold:
1. The host `QProcess` is terminated/killed.
2. `cleanupContainer(containerName)` executes `docker kill <containerName>` and `docker rm -f <containerName>`.
3. Host temporary directories are cleaned up via RAII (`QTemporaryDir`).
4. On application shutdown, `JobManager::shutdown()` stops all running jobs and ensures no orphaned containers remain.

## Permissions & Cross-Platform Compatibility

Different platforms (Linux, macOS with Docker Desktop, Windows with WSL2) handle volume permissions differently:
- Some container images run as root, while others run as an unprivileged user (e.g. `1000:1000`).
- OCTOS sets world-writable permissions on the temporary output directory prior to starting the container, ensuring unprivileged container users can write assembly and bytecode outputs back to the host filesystem.
