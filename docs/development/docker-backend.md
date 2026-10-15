# Docker Execution Backend & Security Sandbox

OCTOS runs compiler toolchains inside short-lived Docker containers. This page describes what the
backend (`src/backend/DockerExecutionBackend.*`, `DockerExecutionConfig.*`) really does, how it
cleans up, and — just as important — what it does **not** protect against.

## Container flags

Every job is started with `docker run` and these flags (see `buildDockerRunArgs()`):

| Flag | Purpose |
| --- | --- |
| `--rm --pull=never` | Remove the container on exit. **Never download an image implicitly**: a missing image fails fast with a *Docker* error; pulling stays an explicit action in the Compiler Manager. |
| `--network none` | No network for regular compiler jobs. |
| `--memory M --memory-swap M` | Hard memory limit, swap disabled (default `512m`). |
| `--cpus C`, `--pids-limit N` | CPU share (default `1.5`) and process limit (default `128`). |
| `--cap-drop=ALL`, `--security-opt no-new-privileges` | No capabilities, no privilege escalation through setuid binaries. |
| `--ipc none` | No shared IPC namespace. |
| `--tmpfs /tmp:rw,noexec,nosuid,size=64m` | Scratch space is a bounded tmpfs (size configurable). |
| `--ulimit fsize=<maxTotalArtifactBytes>` | A process cannot write one file bigger than this. Above the read limit on purpose: artifact *truncation* stays reachable. A violating process gets `SIGXFSZ` (exit 153), which is reported as *Limit exceeded*. |
| `--read-only` (optional) | Read-only root filesystem (`readOnlyRootFs`, off by default for compatibility). |
| `--user uid:gid` (Linux) | Run as the host user, so output files are owned correctly. |
| `-v <in>:/input:ro`, `-v <out>:/output:rw`, `-w /output` | Source is read-only; the only writable host directory is a fresh temporary directory (mode `0750`, never world-writable). |

All settings come from `DockerExecutionConfig`. `fromSettings()` validates **per field**: one corrupt
setting falls back to its own default and no longer resets the others.

These flags are verified against a real daemon by `tests/integration/test_docker_flags.cpp`
(capabilities, `NoNewPrivs`, cgroup memory/swap/pids, no network, bounded tmpfs with `noexec`,
file-size ulimit → exit 153, read-only input).

## Limits on what is read back

| Limit | Default | Behaviour |
| --- | --- | --- |
| `maxSourceBytes` (main file + extra files) | 1 MB | Job fails with *Limit exceeded* before any container starts. |
| `maxOutputBytes` per stream (stdout/stderr separately) | 5 MB | Output is cut at a UTF-8 boundary, a marker is appended, a warning is added. If the process keeps flooding (4× the limit is discarded) it is killed and the job fails with *Limit exceeded*. |
| `maxFileSizeBytes` (one artifact) | 10 MB | Content is the exact prefix; `artifactTruncated` and `CompilationArtifact::truncated` are set, a warning reaches the UI, and the result is **not** `isComplete()`. |
| `maxTotalArtifactBytes` | 20 MB | Raw-output artifact is dropped if the total would exceed it. |

The output file is only read if it is a regular file whose canonical path lies inside the output
directory — a symlink created by the sandboxed process cannot make OCTOS read host files.

## Concurrency model and cleanup

* `execute()` runs entirely on the caller's worker thread and owns its `QProcess`.
* The shared registry (`m_jobs: jobId → {container name, cancelled}`) is touched only for short
  bookkeeping under `m_mutex`. **No docker process is started or awaited under that mutex.**
* A job registers its container name *before* anything can fail. The registration is removed by a
  scope guard only **after** the cleanup has run.
* Cleanup is `docker rm -f <name>` (kill + remove in one step). It is idempotent: "No such container"
  counts as success; any other failure is **not** reported as success. Names that do not start with
  `octos_` are refused, so foreign containers are never touched.
* If cleanup fails, the name moves to the *leaked* set (`leakedContainers()`), the job result carries a
  warning, and `retryLeakedCleanup()` / the destructor retry it.
* `cancel(jobId)` only raises a flag and returns immediately (it is called from the GUI thread). The
  worker polls the flag at least every 100 ms, kills its docker client and performs the authoritative
  cleanup. Cancelling an unknown or finished job is a no-op and leaks nothing.
* If the docker daemon is unreachable no container can exist, so no cleanup (and no warning) is needed.

## Target architecture

`CompilationRequest::targetArchitecture` (probed architecture of a verified image, otherwise the Docker daemon's
architecture) decides whether x86-only options are passed: `-masm=intel|att` (GCC, Clang, GNAT) and
`llvm-args=--x86-asm-syntax` (rustc) are omitted for `aarch64`/`arm64`. An unknown architecture is treated
as x86-64.

## Failure attribution

`CompilationResult::failureOrigin` tells the UI who is responsible:
`Compiler` (the toolchain ran and reported errors), `Docker` (exit 125/126/127, daemon/image errors),
`Limits` (timeout, output flood, size limits, exit 153), `Internal` (OCTOS itself).
`presentResult()` turns this into the status badge ("Compiler error", "Docker error", …).

## Threat model and residual risks

Docker is a *convenience boundary*, **not a complete sandbox for hostile code**.

* **Protected against (reasonably):** accidental damage by compiler invocations, runaway memory/CPU/
  processes/output, network access, writes outside the output directory, privilege escalation inside
  the container with the flags above.
* **Not protected against:** kernel or container-runtime vulnerabilities (container escape), a hostile
  *image* (images are trusted code!), side channels, resource exhaustion of the Docker VM itself
  (e.g. a full Docker Desktop disk makes every `docker run` fail), and denial of service within the
  configured limits. Compilers can execute arbitrary code at compile time (macros, plugins, build
  scripts); that code runs inside the container with the limits above and nothing more.
* **Platform differences:** on Linux the container user is mapped to the host uid/gid; Docker Desktop
  (macOS/Windows) handles ownership through its file-sharing layer. Docker Desktop containers run in a
  VM, which adds an isolation layer that plain Linux does not have.
* Do not run untrusted source code through OCTOS on a machine where a container escape would matter.

## Images, ownership and verification

See [Compiler Registry](compiler-registry.md): an image *name* is not proof of a working toolchain,
and a `gcc:*` image on your machine is not necessarily "owned" by OCTOS.
