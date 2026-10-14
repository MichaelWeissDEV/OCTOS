# Troubleshooting

Common issues and recommended resolutions when running OCTOS:

### Cannot Connect to Docker Daemon
- **Symptom**: Panes display `"Docker process could not be started"` or tests report Docker unavailable.
- **Cause**: Docker Desktop or the system Docker service is stopped, or the socket permissions are restricted.
- **Resolution**:
  - Linux: Run `sudo systemctl start docker` and ensure your user is in the `docker` group (`sudo usermod -aG docker $USER`).
  - macOS / Windows: Start Docker Desktop and wait until the status indicator turns green.

### Compilation Timeout
- **Symptom**: Pane displays `"Compilation timed out after 30 seconds"`.
- **Cause**: Complex optimization flags or resource-heavy templates.
- **Resolution**: Check your flags or reduce heavy recursive macro/template expansions.

### Permission Denied Writing to Output
- **Symptom**: Compiler reports cannot write to `/output`.
- **Cause**: Incompatible container user ID.
- **Resolution**: OCTOS 0.3.0 sets world-writable permissions on the temporary output workspace before container execution. Ensure your host temporary directory (`/tmp` or `$TMPDIR`) is mounted with read/write access.

### No Compiler Images Listed
- **Symptom**: Compiler dropdown says `"No Compiler Selected"`.
- **Cause**: No compiler images are installed in Docker locally.
- **Resolution**: Open **Tools > Docker Compiler Manager**, select a category (e.g. C++, Rust), and click **Pull** to install your chosen compiler version.
