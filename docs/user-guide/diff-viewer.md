# Diff Viewer

**Diff View** (source toolbar, enabled with two or more panes) compares the output of two panes.

```
┌ Pane A: [Pane 1: gcc:13 -O0 ▾]  vs  Pane B: [Pane 2: gcc:13 -O3 ▾] ─────────────────────────┐
│ Compiler: gcc:13 | Flags: -O0 | Type: Assembly     Compiler: gcc:13 | Flags: -O3 | Type: …    │
│ [x] Show only differences [ ] Ignore whitespace [ ] Ignore comments [ ] Normalize local labels│
│ Find: [        ] ▲ ▼      ← Prev Change   Next Change →             [Copy Patch][Export Patch]│
│ Additions: +83 | Deletions: -240 | Unchanged: 12 | Time: 2 ms                                  │
│ Line A │ Assembly (A)            │ Line B │ Assembly (B)                                       │
└────────────────────────────────────────────────────────────────────────────────────────────────┘
```

## Choosing what to compare

Any two panes can be picked from the two lists; the info lines show compiler, flags and artifact type. If the
types differ (assembly vs. bytecode) a warning banner appears: a line-by-line comparison of different
formats is not meaningful.

## Options

| Option | Effect |
| --- | --- |
| Show only differences | hides unchanged rows (view only; the diff itself is unchanged) |
| Ignore whitespace | collapses runs of whitespace before comparing |
| Ignore comments | strips comments using the **syntax of the detected dialect** (below) |
| Normalize local labels | renames compiler labels by order of first appearance |

Changing a computation option cancels the running computation and starts a new one; an older result can
never overwrite a newer one.

### Dialect-aware comments

| Dialect | Comment syntax |
| --- | --- |
| x86 (AT&T / Intel) | `#` (not inside strings) |
| ARM64 | `//`, Apple `;`, and `#` only at the start of a line — `add x0, x1, #42` keeps its operand |
| JVM bytecode | `//` (the `#12` constant-pool references stay) |
| CPython bytecode | none |
| unknown | nothing is stripped (conservative: only whitespace handling) |

The dialect is taken from the artifact type and, for native assembly, detected from the text (x86 vs. ARM64).
If the two sides disagree, nothing is stripped.

### Label normalisation

`.L5` and `.L9` are the same label if they occupy the same *position* in the numbering; but a jump to the
first label is different from a jump to the second. A blanket "replace every `.L…` by `.L_lbl`" would hide
that difference; OCTOS does not do that. Side effect: inserting a new label early shifts the names of later
ones, which can produce extra differences — that is the price of keeping targets distinguishable.

## Computation: exact, coarse or refused

| Situation | What you get |
| --- | --- |
| normal | an exact, **minimal** diff (shortest edit script) |
| step budget exceeded (extremely different huge inputs) | a **coarse block diff** (common prefix/suffix kept, the middle replaced), explicitly flagged *INCOMPLETE* |
| more than 100 000 lines per side, 64 MB, or the memory budget | **no comparison** and a message — OCTOS never silently cuts input and then calls it a result |
| you cancel | partial work is discarded, flagged cancelled |

An incomplete result is never offered as a patch (below). A progress bar and **Cancel** are shown while the
worker runs; the dialog stays usable. Closing the dialog cancels the job.

## Navigation and search

* **← Prev Change / Next Change →** jump between changed rows (wrapping).
* **Find** highlights all rows containing the text; ▲/▼ move between matches; the counter shows `n of m`.
  If a match is in a row hidden by *Show only differences*, the filter is switched off.
* **Ctrl+C** copies the selected rows (tab-separated).
* Added rows are tinted green, removed rows red; the tints work on every theme.

## Patch export

**Copy Patch** / **Export Patch…** produce a standard *unified diff* with 3 lines of context:

* correct `@@ -a,b +c,d @@` ranges (including empty sides), `\ No newline at end of file` handling;
* lines that are equal only *after normalisation* but differ textually are exported as `-A`/`+B`, so the patch
  really turns text A into text B;
* the file is written atomically.

The patch round trip is verified in the test suite, including application with the system `patch` tool.
For an *incomplete* result OCTOS refuses to create a patch and explains why; you may choose to export it anyway,
in which case the file starts with `# OCTOS INCOMPLETE DIFF - NOT A VALID PATCH` and cannot be applied.
