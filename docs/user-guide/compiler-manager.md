# Compiler Manager

**Tools ▸ Docker Compiler Manager…** lists compiler images, downloads new ones, verifies them and deletes
the ones you no longer need.

```
┌ ● Docker Connected: Docker daemon is running and reachable. ───────────────────────────────┐
│ Categories │ Search [          ]  [Refresh]                                                │
│  C++       │ Image / Tag │ Toolchain │ Architecture │ Compatibility          │ Action        │
│  C         │ gcc:13      │ GCC       │ arm64        │ Verified – gcc (GCC) 13│ [Delete][Verify]│
│  Clang     │ gcc:12      │ GCC       │ arm64        │ Plausible (not verified)│ [Download]   │
│  Rust …    │                                                                               │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

## Docker connection banner

Green: the daemon answered `docker info`. Red: it did not (not installed, not running, no permission). The
banner never blocks the window — the check runs in the background, and so does every other Docker query
(`docker images`, `docker rmi`, probes). If listing the images fails, the status line shows the error and
the **previous list is kept**: an error is never presented as "no images installed".

## Categories and tags

Pick a category (C++, C, Clang, Rust, Java, Python, C#). OCTOS asks the **Docker Hub** API for the newest
stable tags of the matching official repository (`gcc`, `silkeh/clang`, `rust`, `eclipse-temurin`, `python`,
`mono`). Tags containing `rc`, `beta`, `alpha`, `nightly`, `dev` or `snapshot` are filtered out. This is the
only place where OCTOS talks to the internet besides the actual `docker pull`.

## The *Compatibility* column — what it really means

| Label | Meaning | How you get there |
| --- | --- | --- |
| **Image recognized** | OCTOS recognised the image *name*. This says nothing about what is inside. | an unknown or experimental image |
| **Plausible (not verified)** | the name and metadata suggest a complete toolchain (known family, required tools listed) | default for supported families |
| **Verified – \<version\>** | a probe ran **inside the image** and found every required tool and a compiler version | click **Verify** |
| **Verification failed** | the probe ran and failed; the tooltip says why | click **Verify** |

**Verify** runs a locked-down container (`--network none`, all capabilities dropped, small memory/PID limits)
that checks for each required binary, prints the architecture (`uname -m`) and the first line of the
compiler's version output. It never downloads anything: if the image is not installed locally the probe
reports exactly that.

The result (version, architecture, image ID, time) is stored and survives restarts. It only counts while
the image ID is still the one that was probed: pulling a new image under the same tag silently invalidates
the old verification and the label falls back to *Plausible*.

Verified images additionally show their **real compiler version and architecture** in the compiler pane
info line.

## Downloading

**Download** asks for confirmation, then runs `docker pull`. A progress bar shows the percentage (parsed
from Docker's output). Only one download runs at a time. A failed pull reports Docker's message. If the
image was **not** installed before the pull, OCTOS remembers that it downloaded it ("owned by OCTOS", see
below).

## Deleting

**Delete** runs `docker rmi` after an explicit confirmation (default answer: *No*).

* An image **OCTOS downloaded itself** → a normal confirmation.
* An image that **was already on your machine** (even if its name is `gcc:13`) → a stronger warning: it may be
  used by other projects or containers. OCTOS never assumes it owns an image because of its name.
* Names that do not look like compiler images are refused outright.
* If a container still uses the image, Docker refuses and the message is displayed.

## Which images appear in the pane's compiler list

Only **installed** images that are compatible with the pane's language: `gcc` and `clang` for C/C++, `rust`
for Rust, `eclipse-temurin`/`openjdk` for Java, `python` for Python, `mono` for C#, `gnat`/`gcc` for Ada.
After a download, a delete or a refresh the lists update automatically.

## Architectures

Docker pulls the variant for your host (`arm64` on Apple Silicon, `amd64` on most PCs). That determines the
assembly you see: on an ARM host you get **AArch64 assembly**, not x86. OCTOS passes x86-only options
(`-masm=`, x86 syntax selection) only for x86 targets.
