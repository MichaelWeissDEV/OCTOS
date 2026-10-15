# Error Reference

What the badges and messages mean, who is responsible, and what to do.

## Status badges

| Badge | `FailureOrigin` / state | Typical cause | What to do |
| --- | --- | --- | --- |
| **Compiler error** | `Compiler` | syntax/semantic error in your code or an unsupported flag | read the Diagnostics table; fix the source/flags |
| **Docker error** | `Docker` | daemon not reachable, image not installed (`--pull=never`), image lacks the compiler (exit 126/127), Docker said "Error response from daemon" | start Docker, install the image in the Compiler Manager, use **Verify** |
| **Limit exceeded** | `Limits` | source too large; output flood (more than 4× the output limit); a process wrote a file bigger than the file-size limit (exit 153); timeout | reduce output/size; raise the timeout is not exposed yet |
| **Timeout** | `TimedOut` (`Limits`) | job ran longer than 30 s | simplify templates/macros, lower optimisation |
| **OCTOS internal error** | `Internal` | temp directories could not be created, no strategy for the language, a rejected project file path, a backend exception | see the message; report it if reproducible |
| **Incomplete** | success, but truncated | output exceeded a size limit | see the notice; the artifact is an exact prefix |
| **Cancelled** | `Cancelled` | you pressed Cancel, closed the pane, or a newer edit replaced the job | none |

## Messages

| Message | Meaning |
| --- | --- |
| `Docker process could not be started. Check if Docker is installed and the daemon is running.` | `docker` could not be launched or did not start within 5 s |
| `Cannot connect to the Docker daemon …` | the daemon is not running or the socket is not accessible |
| `Unable to find image '…' locally` | the image is not installed; OCTOS never pulls during compilation |
| `No compiler strategy available for this language` | internal: no strategy registered |
| `Source is too large (N bytes, limit M bytes)` | main file plus extra project files exceed 1 MB |
| `Rejected project file: …` | an extra project file had an unsafe path (absolute, `..`, symlink escape) or collides with the main file name |
| `Compilation timed out after N seconds` | timeout |
| `Compiler output exceeded the configured limit and the job was stopped` | flood protection triggered |
| `The compiler wrote a file larger than the configured size limit and was stopped.` | `SIGXFSZ` (exit 153) |
| `The compiler could not be executed inside the image (command not found or not executable).` | exit 126/127 with no other output; the image does not contain the compiler |
| `Container octos_… could not be removed; it will be retried.` | warning: cleanup failed (daemon busy/unreachable); the name is tracked and retried |
| `Output artifact was truncated at N bytes; it is incomplete.` | artifact limit hit |
| `Compiler stdout/stderr was truncated.` | stream limit hit |
| `Output artifact ignored: it is not a regular file inside the sandbox.` | the output file was a symlink or escaped the output directory |

## Project and diff messages

| Message | Meaning |
| --- | --- |
| `This project was created by a newer OCTOS (format …)` | the project file's format version is newer than this build supports |
| `Invalid project: …` | validation failed; up to five reasons are listed |
| `Project not saved, it is invalid: …` | an in-memory project violated the rules; the file on disk was not touched |
| `Failed to open … for writing` / `Failed to finalise …` | the atomic save failed; the old file is intact |
| `Input exceeds the line limit …; no comparison was performed` | diff refused (limits) |
| `Diff work limit reached: showing a coarse block-level difference, NOT a minimal or exact diff` | step budget exceeded |
| `No patch exported: …` | the diff is incomplete or cancelled |

## Exit codes seen inside containers

| Code | Meaning | Mapped to |
| --- | --- | --- |
| 0 | success | Completed |
| non-zero from the compiler | compile errors | `Compiler` |
| 125 | `docker run` itself failed | `Docker` |
| 126 / 127 | command in the container not executable / not found | `Docker` |
| 137 | killed (OOM or `docker kill`) | `Compiler` unless cancelled/timeout |
| 153 | `SIGXFSZ`: file-size limit | `Limits` |
