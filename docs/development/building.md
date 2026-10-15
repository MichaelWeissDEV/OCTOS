# Building & Running

## Requirements

| Tool | Version | Notes |
| --- | --- | --- |
| C++ compiler | C++17 (GCC ≥ 9, Clang ≥ 10, MSVC 2019) | |
| CMake | ≥ 3.16 | |
| Qt | ≥ 6.2 (6.9+ recommended) | components `Core Gui Widgets Network Test`. `QPromise` needs 6.1+. Developed against 6.11 |
| Docker | any recent | only needed to *run* compilations and the Docker integration tests |
| Ninja | optional | faster builds |

## Configure and build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/path/to/qt          # e.g. $(brew --prefix qt) on macOS
cmake --build build
./build/OCTOS
```

## CMake options

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_TESTING` | `ON` | build and register the tests |
| `OCTOS_BUILD_BENCHMARKS` | `ON` | build `octos_bench` (not part of ctest) |
| `OCTOS_BUILD_FUZZERS` | `OFF` | build libFuzzer targets (needs a clang with the fuzzer runtime) |
| `CMAKE_BUILD_TYPE` | — | `Debug`, `Release`; tests behave differently in Release (timing, `Q_ASSERT`) so run both |

## Targets

| Target | Kind | Contents |
| --- | --- | --- |
| `octos_core` | static lib | everything without widgets |
| `octos_ui` | static lib | widgets, `MainWindow`, highlighters, themes |
| `OCTOS` | executable | `main.cpp` + resources |
| `test_*` | executables | see [Testing](testing.md) |
| `octos_bench` | executable | benchmarks |
| `fuzz_*` | executables | libFuzzer targets (optional) |

## Running tests

```bash
ctest --test-dir build --output-on-failure
# a single suite with output
QT_QPA_PLATFORM=offscreen ./build/tests/test_project_ui -v1
```

Tests never touch your real settings (`OCTOS_SETTINGS_DIR`) and the Docker tests only use images that are
already present (they skip visibly otherwise).

## Sanitizer builds

```bash
cmake -S . -B build-tsan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=… \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake -S . -B build-asan … -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" …
```

## Fuzzing

```bash
cmake -S . -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=… \
      -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DOCTOS_BUILD_FUZZERS=ON -DOCTOS_BUILD_BENCHMARKS=OFF
cmake --build build-fuzz
ASAN_OPTIONS=detect_leaks=0 build-fuzz/tests/fuzz_asm_parser -max_total_time=60 corpus/
```

## Benchmarks

```bash
build/octos_bench            # all cases, each in a fresh process (clean peak RSS)
build/octos_bench --list
build/octos_bench diff_10k_different
```

## Useful environment variables

See [Environment Variables](../reference/environment.md).

## Code style

`.clang-format` in the repository root. Prefer small functions, RAII, `const` correctness, typed enums and
explicit error objects/messages over booleans. Match the surrounding comment density: comments explain *why*
(a race, a limit, a platform quirk), not what the next line does.

## Debugging tips

* The *Compiler Metadata* artifact shows the exact container command; paste it into a shell to reproduce.
* `OCTOS_DUMP=1 ./test_asm_parser testDumpCorpus` prints functions/blocks/edges of the corpus.
* A fake `docker` script in `PATH` is the quickest way to test failure modes (see `test_docker_backend.cpp`).
* On macOS, `lldb` may be unable to attach (SIP); use sanitizers instead.
