# Data Model Reference

The central types live in `src/core/` and `src/parser/`. This page lists their fields and the invariants the
code (and tests) rely on.

## `CompilationRequest`

| Field | Type | Meaning |
| --- | --- | --- |
| `jobId` | `quint64` | assigned by `JobManager`, unique per process |
| `paneId` | `quint64` | the pane the result belongs to |
| `revision` | `quint64` | document revision the request was made for (stale-result protection) |
| `sourceCode` | `QString` | the main source file |
| `compilerImage` | `QString` | Docker image, e.g. `gcc:13` |
| `language` | `Language` | `Cpp C Rust CSharp Java Python Ada` |
| `syntaxFlavor` | `SyntaxFlavor` | `Intel` / `ATT` (x86 only) |
| `userArgs` | `QString` | free-form flags (sanitised by the strategy) |
| `rustCrateType` | `RustCrateType` | `Auto` / `Binary` / `Library` |
| `outputKind` | `OutputKind` | `Assembly` (default) / `LlvmIr`; honoured only if `strategy->supportsOutputKind()` (Clang, Rust), otherwise assembly is produced |
| `extraFiles` | `QVector<QPair<path, content>>` | additional project files placed next to the main file |
| `mainFilePath` | `QString` | project path of the main file (include directories) |
| `targetArchitecture` | `QString` | `x86_64`, `aarch64`, … ; empty = unknown = x86 behaviour. `targetsX86()` decides whether x86-only options are passed |
| `timeoutMs` | `int` | job timeout (30 000 by default) |

## `CompilationResult`

| Field | Meaning |
| --- | --- |
| `jobId`, `paneId`, `revision` | copied from the request; the scheduler enforces them |
| `state` | `JobState`: `Queued Starting Running Completed Failed Cancelled TimedOut` |
| `exitCode` | compiler/client exit code; `-1` internal/cancelled, `-2` timeout, `-3` output flood |
| `rawOutput`, `assemblyOutput`, `errorOutput` | text views (legacy convenience; artifacts are the structured form) |
| `diagnostics` | parsed `Diagnostic`s (`severity file line column message`) |
| `durationMs` | wall time |
| `compilerImage`, `imageDigest`, `architecture`, `compilerVersion` | environment; version/architecture/digest only when the image was verified |
| `compilerCommand` | the exact compiler invocation inside the container |
| `stdoutTruncated`, `stderrTruncated`, `artifactTruncated` | truncation flags |
| `failureOrigin` | `None Compiler Docker Limits Internal` |
| `warnings` | non-fatal user-visible notes (truncation, container cleanup problems …) |
| `artifacts` | `QVector<CompilationArtifact>` |

Helpers: `success()` (Completed and exit code 0), **`isComplete()`** (success and nothing truncated),
`hasArtifact(type)`, `artifact(type)`.

## `CompilationArtifact`

| Field | Meaning |
| --- | --- |
| `id` | stable within a result: `<job>:<index>:<type>` |
| `type` | `Assembly LLVM_IR JVM_Bytecode CPython_Bytecode CIL Diagnostics CompilerMetadata RawOutput Other` |
| `name`, `content`, `sizeBytes`, `mimeType`, `metadata` | display name, UTF-8 text, size, MIME type, free key/value map |
| `source` | where it came from ("compiler output file", "compiler stdout/stderr", …) |
| `compilerProfile` | image + flags it was produced with |
| `compilationId` | the producing job |
| `createdAt`, `checksumSha256` | creation time (UTC), SHA-256 of the UTF-8 content |
| `truncated` | content is an incomplete prefix |
| `containerPath` | path inside the container (informational) |
| `hostTempPath` | transient host path — **never set by the backend**; the sandbox is gone after the job |
| `persistentPath`, `storage` | durable copy, only after an explicit `saveTo()`; `ArtifactStorage::InMemory` vs `PersistentFile` |

## `FailureOrigin` and `PresentationLevel`

`presentResult()` (in `ResultPresentation.h`, free of widgets) maps a result to the badge text, the severity
(`Ok`, `Warning`, `Error`) and the notice. See [Error Reference](errors.md).

## Assembly model (`AssemblyModule`)

| Type | Fields |
| --- | --- |
| `AssemblyModule` | `architecture`, `syntax`, `functions`, `labels`, `globalDirectives`, `unparsedLines`, `sourceFiles` (id→path), `warnings`, `truncated` |
| `Function` | `name`, `isGlobal`, `declaredAsFunction`, `section`, `basicBlocks`, `edges`, `startLineNumber`, `endLineNumber`, `sizeBytes` (-1 = unknown) |
| `BasicBlock` | `id` (index, stable), `name`, `labels` (aliases), `instructions`, `successors`/`predecessors` (edge indices), line range, compat name lists |
| `BlockEdge` | `from`, `to` (-1 = outside/unresolved), `kind` (`Taken Fallthrough Unconditional IndirectUnknown`), `targetName`, `leavesFunction` |
| `Instruction` | `mnemonic`, `prefix`, `operands`, `rawText`, `sourceLoc`, `originalLineNumber`, `kind`, `isBranch`, `branchTarget` |
| `InstructionKind` | `Normal ConditionalBranch UnconditionalBranch IndirectBranch Call IndirectCall Return Trap Unknown` |
| `Operand` | `rawText` (always kept), `kind` (`Register Immediate Memory Symbol Relocation Unknown`), `registerName`, `symbol`, `relocation`, `indirect`, convenience bools |
| `SourceLocation` | `fileName`, `fileId`, `line`, `column`, `fileKnown` |
| `AssemblyMetrics` | counts, frequencies, `codeSizeKnown`/`codeSizeBytes` |

### Invariants (checked by the tests and fuzz targets)

* `BasicBlock::id` equals its index; every block has at least one instruction.
* A terminator (`jcc`, `jmp`, `ret`, `ud2`, …) is only ever the **last** instruction of a block.
* Return/trap blocks have no successors; a conditional branch has `Taken` + `Fallthrough` (only `Taken` if it
  is the last block); a direct/indirect jump has exactly one edge; other blocks fall through.
* `IndirectUnknown` edges have `to == -1` and no target name; `Fallthrough` always goes to `id + 1`.
* Operand convenience flags are consistent with `kind`; `Unknown` operands carry no semantic flags.
* `successors`/`predecessors` are consistent with `edges`.

## `Workspace`, `CompilerProfile`, `BuildTarget`

See [Project File Format](project-file-format.md) and [Workspaces & Artifacts](../development/workspaces.md).

## `CompilerDefinition` and `ToolchainProbeResult`

`CompilerDefinition` holds id, display name, family, language, version tag, image, digest, architecture,
supported artifact types and syntax flavours, optimisation options, required binaries, an experimental flag, a
status message, the stored probe and the currently installed image id. `verificationState()` derives
`ImageRecognized / ToolchainPlausible / Verified / VerificationFailed`. `ToolchainProbeResult` records
`performed`, `success`, found/missing binaries, `version`, `architecture`, `imageDigest`, `checkedAt`,
`message`.
