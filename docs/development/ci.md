# Continuous Integration

`.github/workflows/ci.yml` builds and tests on Ubuntu 22.04 and macOS (Release, Ninja) with
`QT_QPA_PLATFORM=offscreen`.

## What CI runs

* configure with `-DBUILD_TESTING=ON`, build everything (including `octos_bench`), `ctest --output-on-failure`.
* GUI tests need no display (offscreen platform).

## What CI does not cover (run locally)

| Check | How |
| --- | --- |
| Debug build (Q_ASSERT active) | `-DCMAKE_BUILD_TYPE=Debug` |
| ThreadSanitizer | see [Building](building.md#sanitizer-builds) — run the concurrency tests 20+ times |
| AddressSanitizer + UBSan | same, all tests |
| libFuzzer targets | `-DOCTOS_BUILD_FUZZERS=ON` with a fuzzer-capable clang |
| Docker integration (real compilers) | `test_docker_integration` skips unless images are installed — pull the images you want to check first |
| Real Docker flags | `test_docker_flags` needs a running daemon and a local `alpine:latest` |
| Benchmarks | `octos_bench` in Release |

## Reading test results

`ctest` reports a test with skipped cases as *Passed*. Check the `Totals: … N skipped` line of the
executables that depend on external tools (`test_docker_integration`, `test_docker_flags`,
`test_host_compilers`, `test_registry_and_probe`).

## Platform notes

* Linux packages: `qt6-base-dev` provides Core, Gui, Widgets, Network and Test.
* The Docker-dependent tests are designed to skip, never fail, when Docker or an image is missing.
* The tests do not require network access.
