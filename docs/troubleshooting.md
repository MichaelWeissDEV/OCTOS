# Troubleshooting

Start with the **status badge** of the pane ([Error Reference](reference/errors.md)): it tells you whether the
compiler, Docker, a limit, or OCTOS itself is responsible.

## Docker

### "Docker error": cannot connect to the daemon
* **Symptom**: badge *Docker error*; message `Cannot connect to the Docker daemon …` or `Docker process could
  not be started`; the Compiler Manager banner is red.
* **Cause**: Docker is not running, not installed, `docker` is not in `PATH` for the application, or your user
  has no permission for the socket.
* **Fix**: Linux: `sudo systemctl start docker`, add yourself to the `docker` group
  (`sudo usermod -aG docker $USER`, then log in again). macOS/Windows: start Docker Desktop and wait until it
  is ready. GUI applications on macOS may not inherit your shell `PATH`; `/usr/local/bin/docker` or
  `/opt/homebrew/bin/docker` must be reachable.

### "Docker error": image not found
* **Symptom**: `Unable to find image '…' locally`.
* **Cause**: compiling never downloads images (`--pull=never`). The compiler list only shows installed images,
  but you may have deleted the image outside OCTOS.
* **Fix**: Compiler Manager ▸ **Refresh**, then **Download** the image.

### "Docker error": command not found in the image (exit 126/127)
* **Cause**: the image does not contain the compiler (wrong image for the language, slim image).
* **Fix**: use **Verify** in the Compiler Manager; it lists which required tools are missing.

### `no space left on device` from Docker
* **Cause**: Docker Desktop's virtual disk is full (many images/volumes). Every `docker run` fails.
* **Fix**: free space (`docker system df` shows what uses it; remove what you do not need — OCTOS never prunes
  for you). Not an OCTOS bug.

### Leftover containers `octos_<pid>_<job>`
OCTOS removes its containers after every job and on shutdown; failures are reported in the pane notice and
retried. If the application crashed, remove stragglers manually: `docker ps -a --filter name=octos_`
then `docker rm -f <name>`.

## Compilation results

### Timeout
`Compilation timed out after 30 seconds`: heavy templates or macros at high optimisation. Reduce the code or
the flags.

### "Incomplete"
Output exceeded a size limit (5 MB per stream, 10 MB per artifact). The artifact is an exact prefix. Reduce
the output (fewer instantiations, no huge string tables) — see [Artifacts](user-guide/artifacts.md).

### "Limit exceeded" with exit code 153
A process inside the container tried to write a file larger than the allowed size and was stopped.

### Assembly looks like ARM on my Mac
Apple Silicon hosts pull `arm64` images, so you really get **AArch64** assembly. That is correct. For x86
output you would need an `amd64` image run under emulation, which OCTOS does not configure.

### GCC fails with an unknown option `-masm`
Older builds passed `-masm=` unconditionally; GCC rejects it on aarch64. Current builds pass it only for x86
targets. Make sure you run a current build; check the *Compiler Metadata* artifact for the exact command.

### Rust: "`main` function not found"
Pick the crate type **Library** in a profile, or leave it on *Auto*: without `fn main(` OCTOS builds a library.
If you forced *Binary*, add a `main`.

### Java prints nothing for a packaged class
Fixed: all generated classes (packages, inner and secondary classes) are disassembled. Update OCTOS.

### Python shows no diagnostics for a syntax error
Fixed: the source is `compile()`d first so the `SyntaxError` carries file, line and column.

## Interface

### Nothing compiles while I type
*Tools ▸ Auto-Compile on change* is off. Press **F5**.

### The analysis panel is empty
* The pane shows something other than assembly (bytecode, an error).
* The listing is neither x86-64 nor AArch64 GNU-assembler output.
* The status line of the panel says why (parser limits, no functions found).

### Analysis "jump" does nothing
The target line is hidden by your display filters; the status bar says so. Enable the filters that hide it
(e.g. *Show Metadata Labels*) or navigate manually.

### A project file does not open
The message lists the reasons: unsafe paths, duplicate names, dangling references or a newer format version.

### Light theme has dark panels
That would be a bug — all colours come from theme tokens. Please report with a screenshot.

### Shortcuts do nothing on macOS
Menu shortcuts use ⌘ instead of Ctrl; `Compile Now` is `F5` (may need `fn`).

## Resetting

* **View ▸ Reset Layout** restores window arrangement.
* Delete the settings store ([Settings](reference/settings.md)) to reset everything. Your projects and
  profiles-in-projects are unaffected (they live in `.octos` files).

## Reporting a problem

Include: OCTOS commit/version, OS and CPU architecture, `docker version`, the pane's badge and notice, the
*Compiler Metadata* artifact (exact command), and, for UI problems, a screenshot with the theme name.
