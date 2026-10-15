# Assembly Filters

Raw compiler output contains a lot of bookkeeping. `AssemblyTextProcessor` removes it for display; every
filter can be toggled at any time from the **Filters** menu and takes effect immediately.

| Filter (default) | Shows when on | Typical lines affected |
| --- | --- | --- |
| Show Segment Directives (off) | section switches | `.text`, `.data`, `.section .rodata` |
| Show Data Directives (off) | data/alignment/type directives outside data sections | `.globl`, `.align`, `.type`, `.size`, `.string`, `.long` |
| Show CFI Directives (off) | call-frame information | `.cfi_startproc`, `.cfi_def_cfa_offset`, `.cfi_endproc` |
| Show Metadata Labels (off) | compiler labels | `.LFB0`, `.LFE0`, `.LC0`, `.Lfunc_begin0` |
| Keep Unused Labels (off) | labels nobody references | `.L3:` with no jump to it |
| Hide Empty Labels (off) | *hides* labels that have no code after them | consecutive labels |
| Show Debug Info (.loc) (off) | DWARF line directives | `.loc 1 5 3`, `.file` |
| Show Comments (off) | comments | `# …`, `; …`, `// …` |
| Demangle Identifiers (**on**) | readable C++ names | `_Z3fooi` → `foo(int)` |

## Rules worth knowing

* Data directives are kept **inside data sections** (e.g. the bytes of `.rodata`) even when the filter is off,
  so tables and strings stay readable.
* A label is kept if it is referenced by a jump/call/data reference, unless *Keep Unused Labels* is on.
* The `.loc` directives are always *read* (for source mapping) even when hidden.
* Runs of blank lines collapse to one.
* The filters only change the **display**. The unfiltered text is what the analysis panel parses and what the
  diff compares, so toggling a filter never changes analysis results.
* Filters apply to assembly only. LLVM IR, bytecode and raw output are shown as produced.

## Demangling

Demangling pipes the identifiers through the **host's** `c++filt` (not the container's). If `c++filt` is not
installed or takes longer than 2 seconds, names are left mangled — nothing else changes. Mangled names are
what the [analysis panel](analysis.md) lists, because function names come from the raw text.
