# Assembly Parser & Control-Flow Graph

`AssemblyParser::parseX86_64()` (`src/parser/`) turns GNU-assembler x86-64 output (GCC/Clang, AT&T or
Intel syntax) into an `AssemblyModule`: functions, basic blocks, explicit CFG edges, operands, source
locations. It is the single structured source for the analysis panel and the metrics.
`AssemblyTextProcessor` (display filters) is a separate, older text pipeline.

## Functions

* If the listing contains `.type sym, @function`, **only** those symbols start functions. Other global
  labels (data markers, `.cold` stubs …) stay inside the current function.
* Without any `.type` information (hand-written fragments) every non-local label starts a function.
* `.size sym, N` closes the function; a numeric `N` is kept as `sizeBytes`, an expression such as `.-sym`
  is *not* guessed (`-1` = unknown).
* Sections are tracked (`.text`, `.section …,"ax"`, `.pushsection/.popsection/.previous`). Labels in
  data sections (jump tables, strings, debug info) are labels only, never code.
* Local labels: `.L*` and numeric `1:` (with `1f`/`1b` references).

## Basic blocks and edges

A block starts at the function entry, at every **referenced** jump target, and after every terminator.
Unreferenced labels become aliases of their block (`.Ltmp0` does not split blocks).

| Instruction kind | Edges |
| --- | --- |
| conditional jump (`jcc`, `loop*`, `jcxz`) | `Taken` + `Fallthrough` |
| direct `jmp` | `Unconditional` (flagged `leavesFunction` for a tail call) |
| indirect `jmp`, jump tables | one `IndirectUnknown` edge with **no target** — never an invented one |
| `ret` family, `ud2`, `hlt` | none |
| `call` / indirect `call` | **no** edge and no block split — a call does not end intra-procedural flow |

## Operands

`parseOperand()` classifies `Register`, `Immediate`, `Memory`, `Symbol`, `Relocation` (`foo@PLT`) or
`Unknown`, always keeping the original text. A name that merely *starts* like a register
(`result_value`) is not a register. In AT&T a bare symbol is a memory reference, as a branch target it
is a symbol; in Intel a bare identifier is only classified as a branch target. Anything else stays
`Unknown` and carries no invented properties. ARM64 `#42` immediates are not mistaken for x86.

## Source mapping

`.file N "dir" "name"` and `.loc N line col flags…` are interpreted; the original assembly line numbers
are preserved. An undeclared file id keeps the line but marks the file `fileKnown=false`; an
unparsable `.loc` clears the location (and adds a warning) instead of reusing a stale one.

## Limits

Input limits (`ParseLimits`: 2 M lines, 64 KB per line) set `truncated`; the analysis panel says so.
`;`-separated statements are not split. ARM64 and other architectures are not parsed yet.

## Metrics

`computeAssemblyMetrics()` derives function/block/instruction/edge counts, jump/call/return counts,
tail calls and mnemonic frequencies from the model. **Code size is reported only if every function has a
literal `.size`**; it cannot be derived from assembly text. Counts describe the listing, not runtime
behaviour — fewer instructions do not imply faster code.

## Tests

`tests/unit/test_asm_parser.cpp` checks graph *structure* (diamond, back edge, jump table, tail call,
calls inside one block, boundaries) against **real Clang output** in `tests/data/asm/` (x86-64 ELF,
`-O0`/`-O3`, AT&T and Intel) and enforces CFG invariants on every function. There is no real GCC corpus:
the development host has no GNU GCC (see the README in `tests/data/asm`).
