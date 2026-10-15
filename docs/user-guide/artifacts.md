# Artifacts

A compilation produces several *artifacts*. The **Artifact** selector in the pane header (shown when there is
more than one view) chooses which one you look at.

| Artifact | Produced by | Shown as |
| --- | --- | --- |
| **Assembly** | GCC, Clang, Rust (and GNAT) | text with assembly highlighting, filters, source mapping |
| **LLVM IR** | Clang (`-S -emit-llvm`) and Rust (`--emit=llvm-ir`) when the pane's **Output** is *LLVM IR* | LLVM IR highlighting |
| **JVM Bytecode** | Java (`javap -c -v -p` of *every* generated class) | bytecode highlighting |
| **CPython Bytecode** | Python (`dis`) | opcode highlighting; labelled *not native assembly* |
| **CIL** | C# (`monodis`) | CIL highlighting |
| **Diagnostics** | all compilers | a table instead of text (below) |
| **Compiler Metadata** | OCTOS | JSON: image, exact command, exit code, state, duration, limits |
| **Raw Output** | all | stdout + stderr as the compiler printed them |

Which artifact opens first: a successful compilation opens its first code artifact (assembly / IR /
bytecode); a failed one opens the diagnostics (or the raw error text if there are none).

## Choosing assembly or LLVM IR

Each pane has an **Output** selector: *Assembly* (default) or *LLVM IR*. It is enabled for **Clang images** (C/C++) and
**Rust**; for GCC and the other languages it is disabled and the pane always produces assembly (a stale IR choice
never leaks into a GCC compilation). For IR the assembly *syntax* selector is disabled — IR has no Intel/AT&T
flavour. The IR artifact is the file `output.ll` (the artifact name tells you), is highlighted with IR rules, is not
run through the assembly filters, and is not analysed by the assembly analysis panel. A diff between an assembly pane
and an IR pane shows the usual "different artifact formats" warning. Profiles store the choice in *Output*.

## Diagnostics table

Columns *Severity · File · Line · Col · Message*. **Double-click** a row to jump to that line in the source
editor. The same diagnostics appear as gutter dots and wavy underlines in the editor; hover the dot for the
message. Markers of all panes are merged (duplicates removed). Diagnostics come from parsing the compiler's
text (`file:line:col: severity: message` for GCC/Clang, `error[E…]: … --> file:l:c` for rustc, `File "…", line
N` for Python, `File.java:N: error:` for javac), so unusual message formats may show up in *Raw Output* only.

## Truncated and incomplete output

Limits protect your machine (defaults: 5 MB per output stream, 10 MB per artifact). When something is cut:

* the badge becomes **Incomplete**, a notice explains what was truncated;
* the artifact is marked `(incomplete)` in the selector;
* the content is an exact *prefix* — OCTOS does not append marker text to artifacts (stdout/stderr get an
  inline `[OCTOS: … truncated]` marker);
* an incomplete artifact is never presented as a complete result.

## Saving

**Save…** writes the shown artifact (atomically). This is the only way an artifact gets a durable file path:
the temporary sandbox the compiler used is deleted as soon as the job ends, so OCTOS never offers a path
inside it. Saving a truncated artifact warns you that the file is incomplete.

## Provenance and identity

Every artifact carries a stable id (`<job>:<index>:<type>`), the producing job, the compiler profile
(image + flags), a SHA-256 of its content, its creation time and the size. These are what make a compilation
reproducible: the *Compiler Metadata* artifact records the exact invocation, e.g.
`g++ -S -fno-asynchronous-unwind-tables -masm=intel -g -O2 /input/source.cpp -o /output/output.s`.
