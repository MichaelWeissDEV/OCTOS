# Assembly Filters

OCTOS includes an assembly post-processor (`AssemblyTextProcessor`) that strips boilerplate and compiler noise from the generated output.

Filters can be toggled at any time from the **Filters** menu:

- **Show Segment Directives**: Toggles segment declarations such as `.text`, `.data`, `.section .rodata`. (Default: Off)
- **Show Data Directives**: Toggles alignment and type definitions such as `.globl`, `.align`, `.type`, `.size`, `.string`. (Default: Off)
- **Show CFI Directives**: Toggles Call Frame Information directives like `.cfi_startproc`, `.cfi_def_cfa_offset`, `.cfi_endproc`. (Default: Off)
- **Show Metadata Labels**: Toggles compiler-generated labels like `.LFB0`, `.LFE0`, `.LC0`. (Default: Off)
- **Keep Unused Labels**: Hides labels that are never referenced by any jump, call, or data reference. (Default: Off)
- **Hide Empty Labels**: Hides consecutive or empty labels that contain no code instructions. (Default: Off)
- **Show Debug Info (.loc)**: Toggles raw DWARF `.loc` source directives. (Default: Off; OCTOS parses these directives into source-to-assembly line mappings).
- **Show Comments**: Toggles assembly comments starting with `#`, `;`, or `//`. (Default: Off)
- **Demangle Identifiers**: Demangles mangled C++ symbol names (e.g. `_Z3fooi` becomes `foo(int)`). (Default: On)
