# Limitations

These are the known limits of the current development state (version string `0.4.1`; nothing here is a
release promise).

- **Docker is required for live compilation**, and it is *not* a perfect sandbox: see the
  [threat model](development/docker-backend.md#threat-model-and-residual-risks). Do not compile hostile code
  on a machine where a container escape matters.
- **Docker image verification is explicit.** A recognised image name is not "verified"; use *Verify* in the
  Compiler Manager. Compiling never downloads an image (`--pull=never`).
- **Language support levels**: Ada is *planned*, C# is *experimental* (needs `mcs`/`monodis` in the image).
  Python output is CPython bytecode, not native assembly.
- **Apple Silicon / ARM hosts**: Docker images are then native `aarch64`. OCTOS passes `-masm=`/the x86 assembly syntax option only for x86 targets (GCC rejects `-masm=` on aarch64), but the **Assembly Analysis panel is empty for ARM64 output** (it says so), and the diff dialect detection for ARM64 is heuristic.
- **Assembly parser / analysis**: x86-64 GNU-assembler output only. ARM64, Mach-O and MSVC output are not parsed.
  Jump tables are shown as *unknown* indirect targets; function sizes are only known from literal `.size`.
- **Projects compile one translation unit** (the main file); other files are available as includes. There is
  no CMake/Cargo build inside containers.
- **Diff**: inputs beyond the line/byte/memory budget are refused, and extremely different inputs beyond the
  step budget produce a flagged, coarse block diff. Label normalisation renames labels by order of first
  appearance, so inserting a new label early shifts later names.
- **Docker Desktop**: when its virtual disk is full every `docker run` fails ("no space left on device");
  OCTOS reports it as a Docker error but cannot fix it.
- **Test coverage gaps**: there is no real GNU GCC corpus, and `test_docker_integration` only exercises
  compilers whose images are installed locally (never pulled automatically).
