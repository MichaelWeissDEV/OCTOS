# Glossary

| Term | Meaning |
| --- | --- |
| **Artifact** | one output of a compilation (assembly, IR, bytecode, diagnostics, metadata, raw output) |
| **Basic block** | a straight-line sequence of instructions with one entry and one exit |
| **CFG** | control-flow graph: basic blocks plus edges |
| **Debounce** | wait for typing to pause (300 ms) before compiling |
| **Dialect** | the comment/operand syntax family of a listing (x86 AT&T, x86 Intel, ARM64, JVM, CPython) |
| **Fallthrough** | edge to the next block when a branch is not taken or a block just ends |
| **Image (Docker)** | a read-only template from which a container is started; here: a compiler toolchain |
| **Job / revision** | a compilation request / the document version it was made for |
| **Leaders** | the first instructions of basic blocks |
| **Myers diff** | an algorithm for the shortest edit script; OCTOS uses the linear-space variant |
| **Owned image** | an image OCTOS itself downloaded (as opposed to one that was already there) |
| **Probe** | a short container run that checks which tools an image contains |
| **Profile** | a saved compiler configuration (image, flags, language, syntax) |
| **Revision** | counter increased on every edit; results of older revisions are discarded |
| **Sandbox** | the restricted container a compilation runs in (not a perfect security boundary) |
| **Tail call** | a direct jump to another function instead of a call + return |
| **Target (build)** | main file plus the files built with it |
| **TU** | translation unit: one source file after includes |
| **Verified** | an image whose tools and version were confirmed by a probe |
