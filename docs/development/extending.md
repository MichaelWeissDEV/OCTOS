# Extending OCTOS

Step-by-step recipes. Each recipe ends with the tests you should add.

## Add a language

1. **Enum**: add the value to `Language` in `core/CompilerEnums.h` (+ `languageToString`, `stringToLanguage`).
2. **Strategy**: create `src/strategies/<Name>Strategy.{h,cpp}` implementing `ILanguageStrategy`:
   * `sourceFileName(source)` — file name inside `/input`;
   * `outputFileName()` — file the compiler writes in `/output` (or unused if you read stdout);
   * `buildCompilerArgs(req, inputPath, outputDir)` — the **full** command, first element is the binary;
     pass user data as positional parameters of a script, never interpolate;
   * `sanitizeUserFlags` — reuse `StrategyUtils::sanitizeUserFlags` plus language-specific forbidden flags;
   * `processOutput(req, exitCode, stdout, stderr, fileContent)` — fill `state`, `assemblyOutput`,
     `errorOutput`, `diagnostics`; keep your hands off flags the backend owns (truncation, origin);
   * `parseDiagnostics`.
   Respect `req.targetsX86()` before passing x86-only options.
3. **Registry**: register it in `LanguageStrategyRegistry` (`getStrategy`, `supportsSyntaxFlavor`) and add
   `CompilerRegistry` metadata (family, required binaries, artifact types, default image, version command in
   `ToolchainProbe`).
4. **Artifact type**: if the output is not assembly, decide the `ArtifactType` in
   `DockerExecutionBackend::execute` and add highlighting in `ArtifactHighlighter` if it is new.
5. **UI**: add the language to the pane's language combo, `MainWindow::onLoad` extension mapping, the
   compatibility switch in `CompilerRegistry::isImageCompatibleWithLanguage`, and the profile dialog list.
6. **CMake**: add the files to `OCTOS_CORE_SOURCES/HEADERS`.

**Tests**: argument-vector tests in `test_compiler_strategies.cpp`; a host-compiler test in
`test_host_compilers.cpp` (skips if the tool is missing) that runs the vector against the real tool and checks
the output *and* the diagnostics of a failing input; a property in `fuzz_bodies.h::compilerArgs`.

## Add a CPU architecture to the parser

The parser is architecture-parameterised (`parseImpl(text, arch, …)`); x86-64 and AArch64 are done. For a new one:

1. Add the value to `TargetArchitecture`.
2. In `AssemblyParser.cpp` add: a comment stripper, `parseOperand<Arch>`, `classifyMnemonic<Arch>` (map branch /
   call / return / trap families to `InstructionKind`), and the branch-target operand index (x86: first; AArch64
   `cbz`/`tbz`: last).
3. Extend `detectArchitecture` and `parse()` dispatch; extend `AssemblyDiffProcessor::detectNativeDialect` and
   the opcode database if you want diff dialect and tooltips.
4. **Corpus**: produce *real* compiler output for the architecture into `tests/data/asm/` (document the command
   in the README there) — never hand-write the main corpus.
5. **Tests**: `checkInvariants` over every function of the corpus, structural assertions for if/else, loops,
   switch, tail call, indirect jump, call-doesn't-split-block; operand classification tests including the
   "looks like a register but isn't" cases; extend `fuzz_bodies.h`.

## Add an artifact type

1. `ArtifactType` + `artifactTypeToString` in `core/CompilationArtifact.h`.
2. Produce it in the backend (`finalize(art, source)` fills id, provenance, checksum).
3. Add it to the pane: `isCodeArtifact()` in `CompilerOutputPane.cpp` (if it should open by default),
   highlighting in `ArtifactHighlighter::buildRules`, a `Save…` suffix in `onSaveArtifact`.
4. Document it in `user-guide/artifacts.md` and `reference/data-model.md`.

## Add a display filter

`FilterSettings` + `AssemblyTextProcessor::processAssembly` + a checkable action in `MainWindow::setupUi` +
`onRefilterOutput` + `showSelectedArtifact`. Filters must only change the **display**, never the analysis or
the diff (they use the raw text).

## Add a project-file field

1. Add it to `Workspace`/`CompilerProfile`/`BuildTarget` and to `toJson`/`fromJson` **as optional with a
   default** (older files must keep loading).
2. Validate it in `Workspace::validate()` if it references or contains paths.
3. If it is *required* or changes meaning, bump `kCurrentFormatVersion`, add a migration step in `fromJson`,
   keep reading the old format.
4. Tests in `test_workspace.cpp`: round trip, missing field, hostile value, and add the field to the JSON
   mutation fuzz seeds.

## Add a setting

`AppSettings` getter/setter with a clamped default (+ doc in `reference/settings.md`). Tests that write
settings are safe: every test process uses `OCTOS_SETTINGS_DIR`.

## Add a menu action

Give it a unique shortcut (the `testSaveProjectShortcutIsUnambiguous` test fails on duplicates — Qt disables
both actions of an ambiguous pair), a tooltip, and a test that triggers it.

## Add a theme

See [Theme System](theme-system.md#adding-or-changing-a-theme).

## Checklist before you open a pull request

* `ctest` passes in **Debug and Release** (Release removes `Q_ASSERT`, changes timing).
* Skips are intentional: read the `Totals:` lines.
* New concurrency code ran under ThreadSanitizer several times.
* New parsing code has a property in `fuzz_bodies.h`.
* The docs page of the feature is updated; `CHANGELOG.md` has an `Unreleased` entry.
* No version bump, no tags.
