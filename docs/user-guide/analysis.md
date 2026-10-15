# Assembly Analysis

**View ▸ Assembly Analysis** opens a dock that turns the *raw* assembly of a pane into structure. It is built
from one parse of the listing (the same parser the tests verify), not from display-filtered text and not from
independent regexes.

Supported: **x86-64** (AT&T and Intel) and **AArch64** GNU-assembler output, as produced by GCC and Clang. The
architecture is detected from the text and shown in the summary line. Other architectures and formats are not
parsed (the panel then reports "No functions found").

```
Pane: [Pane 1: gcc:13 -O3 ▾]   x86-64 · 9 functions · 25 blocks · 99 instructions
┌ Functions & Blocks │ Statistics ┐
│ Function  Blocks Instr. Calls Branches Asm lines │ Basic block   Instr. Asm lines Source lines Successors │
│ loop_sum    9     39     0      6      53–179    │ bb0 loop_sum    2    …          …           Taken → .LBB2_1; Fallthrough → … │
```

## Pane selector

With several compiler panes, choose which pane's assembly is analysed. A pane that currently shows something
other than assembly (bytecode, an error) is removed from the list. The analysis runs on a worker thread; newer
requests cancel older ones.

## Functions

One row per function: number of basic blocks, instructions, calls, branches and the line range in the **raw**
listing. Click a header to sort. **Double-click** a function to jump to it in the pane's assembly view (if the
line is hidden by a filter, the status bar says so).

How functions are found:

* If the listing declares functions (`.type name, @function` or `%function`), exactly those symbols start
  functions. Other global labels inside a function (data markers, `.cold` stubs) do **not** split it.
* Without any such declaration (hand-written fragments) every non-local label starts a function.
* `.size name, N` closes a function. Labels in data sections (`.rodata` jump tables, strings, debug info) are
  never code.
* The tooltip of a function says whether it was declared or inferred.

## Basic blocks and successors

Selecting a function lists its blocks. A block starts at the function entry, at every **referenced** jump
target and after every terminating instruction. A label nobody jumps to (e.g. `.Ltmp0`) is only an alias — it
does not cut the block.

The *Successors* column uses these edge kinds:

| Edge | Meaning |
| --- | --- |
| `Taken → X` | the branch is taken (conditional jumps: `jcc`, `cbz`, `b.eq`, …) |
| `Fallthrough → X` | execution continues in the next block |
| `Unconditional → X` | direct `jmp` / `b` |
| `Unconditional → f (leaves function)` | a **tail call**: a direct jump to another function; the block has no other exit |
| `IndirectUnknown → ? (computed jump, targets unknown)` | `jmp *%rax`, `br x16`, jump tables. OCTOS **never** invents targets for these |
| *(none)* | `ret`, `ud2`/`hlt`, `brk`/`udf` |

**Calls do not split blocks and have no edge**: a call returns to the next instruction, so intra-procedural
flow continues.

*Asm lines* are raw listing lines. *Source lines* come from `.loc` directives (`unknown` if there is none).
**Double-click** an *Asm lines* cell to jump in the assembly, a *Source lines* cell to jump into your source.
Unknown mappings are never guessed.

## Statistics

* totals: functions, blocks, CFG edges, instructions;
* conditional / unconditional / indirect jumps, calls, returns, tail calls;
* **code size** — only shown if *every* function has a literal `.size N`; assembler text does not contain
  encoded sizes, so otherwise it says "unknown (not derivable from assembly text)";
* a sortable table of mnemonic frequencies.

!!! warning "Counts are not performance"
    "Fewer instructions" does not mean "faster". Speed depends on the CPU, dependencies, memory, vectorisation
    and more. These numbers describe the *listing* only.

## Limits

Inputs beyond the parser limits (2 000 000 lines, 64 KB per line) are processed partially and the panel says
so. `;`-separated statements on one x86 line are not split.
