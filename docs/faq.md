# FAQ

**Is my code sent anywhere?** No. Compilation happens in local containers with the network disabled. The only
network traffic is Docker Hub tag discovery and image downloads that *you* start in the Compiler Manager.

**Why Docker?** It lets OCTOS offer many compiler versions without installing them, and it contains
accidents (runaway memory, processes, output). It is **not** a perfect sandbox for hostile code, see the
[threat model](development/docker-backend.md#threat-model-and-residual-risks).

**Why does the first compile fail with a Docker error?** Because compiling never downloads images. Install
one in the Compiler Manager first.

**What does "Verified" mean?** A probe ran inside the image and found all required tools plus a version.
A merely recognised image name is *not* verified.

**Why is the assembly ARM on my Mac?** Docker uses native images on your CPU. See
[Troubleshooting](troubleshooting.md#assembly-looks-like-arm-on-my-mac).

**Can I compile several files / use headers?** Yes, with [projects](user-guide/projects.md). One main file is
compiled; the other files are available to `#include`.

**Can I compare two compilers on different languages?** The source is shared between panes, so panes should
use compatible languages. Use two windows for different languages.

**Why was my diff "incomplete"?** Extremely different huge inputs exceed the step budget; you get a flagged
coarse diff, never a silent wrong one. See [Diff Viewer](user-guide/diff-viewer.md).

**Are "fewer instructions" faster?** Not necessarily. The statistics describe the listing only.

**Is LLVM IR supported?** Yes for Clang and Rust: set the pane's **Output** to *LLVM IR* ([Artifacts](user-guide/artifacts.md#choosing-assembly-or-llvm-ir)). GCC cannot produce IR.

**Which architectures can the analysis read?** x86-64 and AArch64 GNU assembler output.

**Where are my settings?** [Settings](reference/settings.md); override with `OCTOS_SETTINGS_DIR`.

**How do I run the tests?** [Testing](development/testing.md).

**Does OCTOS write to my files?** Only the files you save (source files, `.octos` projects, patches,
artifacts) — always atomically, so a failed save leaves the old file intact.
