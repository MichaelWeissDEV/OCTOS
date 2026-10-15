# Diff Engine & Diff Dialog

## Algorithm (`AssemblyDiffProcessor`)

Lines are normalised, interned to integer ids and compared with a **linear-space divide-and-conquer
Myers** algorithm (middle snake). Memory is O(N+M) — it no longer stores one V-vector per edit step
(the old implementation needed O(D²) integers for very different inputs).

Budgets (`DiffOptions`): `maxDiffSteps` (work), `maxLines`, `maxInputBytes`, `maxMemoryBytes`
(estimated). Nothing proportional to the input is allocated before these checks.

* Cancellation is polled every 4096 work units (also inside the Myers loops) and while tokenising.
* Over `maxLines`/`maxInputBytes`/`maxMemoryBytes`: **no comparison** is performed
  (`isTruncated`, empty entries) — an input is never silently cut and then reported as a result.
* Over the step budget: a **coarse block-level difference** (common prefix/suffix + replaced middle) is
  returned, flagged `isApproximate` and `isTruncated` with an explicit status message.
* `DiffResult::isComplete()` is true only for an exact comparison. Cancelled or truncated results are
  never presented as complete.
* Repeated instructions are handled deterministically.

## Normalisation

Comment syntax depends on the dialect (`AsmDialect`): x86 AT&T/Intel → `#`; ARM64 → `//`, `;`, and
`#` only at the start of a line (so `add x0, x1, #42` keeps its operand); JVM → `//` (the `#12`
constant-pool references stay); CPython `dis` → no comments. Strings are respected. **Unknown dialect →
conservative: only whitespace handling.** `detectNativeDialect()` guesses x86 vs ARM64 from content and
returns `Unknown` when ambiguous.

Label normalisation renames compiler labels (`.L*`, Apple `LBB*`) **by order of first appearance per
document**, so `.L5`/`.L9` match structurally but different jump targets stay different (the old
blanket `.L_lbl` replacement hid that). It only runs for known GAS dialects.

## Unified diff export

`toUnifiedDiff()` produces real hunks (3 lines of context, correct `@@` ranges including empty sides,
`\ No newline at end of file`). Lines that are equal only after normalisation are exported as `-A/+B`
so the patch really turns text A into text B. The tests apply the patch with their own applier **and
with the system `patch`**. For an incomplete result no patch is created (empty string + error), or — only
when asked — a banner makes the output unusable as a patch.

## Dialog

`DiffDialog` computes on a worker thread (`QPromise`/`QFutureWatcher`, one job per option change; older
jobs are cancelled and can never overwrite newer results), shows progress and a Cancel button, and uses
a virtual `QAbstractTableModel` (`DiffTableModel`) with a filter proxy instead of four `QTableWidgetItem`s
per row. Closing the dialog cancels the running job. Search, change navigation, copy (Ctrl+C), patch
export (atomic `QSaveFile`) and the compiler info are kept.

## Tests and benchmarks

`test_diff_engine` verifies minimality against an LCS reference on random inputs, reconstruction of both
texts, budgets, cancellation, dialect rules and the patch round trip. `test_diff_dialog` covers
asynchrony ("newest request wins", closing while computing) and the virtual model. Timings live in the
benchmark target `octos_bench`, never in functional tests.
