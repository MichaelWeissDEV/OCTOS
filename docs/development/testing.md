# Testing, Sanitizers, Fuzzing & Benchmarks

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build build
ctest --test-dir build --output-on-failure
```

GUI tests run on Qt's `offscreen` platform (set by CTest). **A skipped test is not evidence**: read the
`Totals:` line of each test (`... 0 skipped`), `ctest` reports skipped tests as "Passed".

## Suites

| Test | Covers |
| --- | --- |
| `test_job_manager` | scheduler, bounded queue, cancellation, shutdown (incl. unresponsive backend) |
| `test_docker_backend` | backend with a scripted fake `docker` (deterministic, no Docker needed): cleanup, idempotence, cancel never blocks, races, truncation regression, output flood, artifacts |
| `test_docker_flags` | **real daemon**: every hardening flag, container cleanup, fail-fast for a missing image. Needs a local `alpine:latest`; never pulls |
| `test_docker_integration` | real compilers in Docker images; skips (visibly) if Docker or the image is missing |
| `test_host_compilers` | strategy argument vectors against compilers on the *host* (rustc, clang, javac/javap, python3) — real compiler output without Docker |
| `test_asm_parser`, `test_parser_and_workspace` | parser/CFG/metrics on real Clang output |
| `test_diff_engine`, `test_diff_dialog`, `test_diff_and_highlighters` | diff engine (LCS reference, patch round trip incl. system `patch`), dialog asynchrony |
| `test_workspace`, `test_project_controller` | validation, version gate, atomic save, traversal, symlinks, dirty state, profiles |
| `test_registry_and_probe` | verification states, probe against `bash:latest`, ownership, async image management |
| `test_theme`, `test_main_window`, `test_project_ui` | contrast of every theme, per-theme screenshots (polarity check), result presentation, artifact viewer, tabs, find/replace, analysis panel, layout persistence |
| `test_fuzz_properties` | the fuzz-target bodies on mutated inputs |

**Settings isolation:** every test executable is built with `OCTOS_TEST_MAIN` (`tests/TestSupport.h`), which sets
`OCTOS_SETTINGS_DIR` to a throw-away directory before anything else runs. `AppSettings` and
`RecentFilesManager` then use an INI file there (`octosSettingsFormat()`), so a test run never reads or
writes your real preferences. (On macOS the native store is CFPreferences, which
`QStandardPaths::setTestModeEnabled` does **not** redirect; an earlier revision of the tests created
`~/Library/Preferences/com.octos.OCTOS.plist` this way. `testSettingsAreIsolatedFromUserPreferences` guards it.)

## Sanitizers

```bash
cmake -S . -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
ctest --test-dir build-tsan -R "job_manager|docker_backend|diff_dialog|project_ui|main_window"
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

There are **no ThreadSanitizer suppressions**. Qt's work queue (`QThreadPool::start`) is not instrumented, so
TSan cannot see that a runnable built on the GUI thread is safely handed to a worker. That edge is stated
explicitly for exactly one address with `OCTOS_TSAN_RELEASE(task)` before the hand-over and
`OCTOS_TSAN_ACQUIRE(this)` at the top of `run()` (`src/utils/TsanAnnotations.h`; no-ops outside TSan builds).
Results travel from the scheduler's workers to the GUI thread through a mutex-protected inbox
(see [Job Manager](job-manager.md)), so only a data-free "drain" call crosses Qt's event queue.

## Fuzzing

`tests/fuzz/` has libFuzzer targets for the assembly parser, operand parser, diff engine (including
abrupt cancellation), workspace JSON and compiler-argument handling. They need a clang with the fuzzer
runtime:

```bash
cmake -S . -B build-fuzz -G Ninja -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
      -DCMAKE_PREFIX_PATH=/path/to/qt -DOCTOS_BUILD_FUZZERS=ON
cmake --build build-fuzz && build-fuzz/tests/fuzz_diff_engine -max_total_time=60 corpus/
```

The properties (e.g. "a complete diff reproduces both inputs", "accepted projects validate", "Rust gets
exactly one `--emit`/`--crate-type`/`--edition`") live in `tests/fuzz/fuzz_bodies.h` and also run in
`test_fuzz_properties`.

## Benchmarks

`octos_bench` (target, not part of ctest) runs each case in its own process and prints wall time and peak
RSS: diff at 1k/10k/50k lines (similar and entirely different inputs), parser at 1.3k/134k/1.3M lines.
There are **no pass/fail thresholds**; use a Release build.
