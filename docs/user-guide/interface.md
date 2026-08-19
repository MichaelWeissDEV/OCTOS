# User Interface

The OCTOS application window is divided into several main areas:

## Main Window Layout

### Source Code Editor (Left Pane)

The left side of the window contains the source code editor where you write the code to be compiled.

**Features:**
- **Syntax Highlighting**: C++ syntax highlighting is applied to the source code
- **Line Numbers**: Line numbers are displayed in the left margin
- **Auto-bracketing**: When you type an opening bracket `(`, `[`, `{`, `"`, or `'`, the corresponding closing bracket is automatically inserted
- **Code Folding**: Support for folding code blocks (implementation dependent on Qt version)

### Compiler Output Panes (Right Side)

The right side of the window contains one or more compiler output panes. Each pane displays the assembly or bytecode output from compiling the source code with a specific compiler and set of flags.

**Pane Controls:**
- **Language Selector**: Choose the programming language (C++, C, Rust, Java, C#, Python, Ada)
- **Syntax Selector**: Choose between Intel or AT&T assembly syntax (where applicable)
- **Compiler Selector**: Choose which Docker compiler image to use
- **Flags Input**: Enter compiler flags (e.g., `-O2`, `-march=native`)
- **Close Button**: Remove this pane

### Toolbar and Menu

**Main Toolbar:**
- **Highlight Toggle**: Cycle through highlighting modes (Off, All, Selection)
- **+ Compare Button**: Add a new compiler output pane for side-by-side comparison
- **Close Others Button**: Close all panes except the current one

**File Menu:**
- **New**: Clear the current source code
- **Open**: Load source code from a file
- **Save**: Save the current source code
- **Recent Files**: Access recently opened files
- **Clear Recent Files**: Remove the recent files history

**Filters Menu:**
Toggle various assembly output filters:
- **Show Segment Directives**: Display `.section`, `.text`, `.data` etc.
- **Show Data Directives**: Display `.byte`, `.word`, `.long` etc.
- **Show CFI Directives**: Display Call Frame Information directives
- **Show Metadata Labels**: Display compiler-generated labels
- **Keep Unused Labels**: Keep labels that aren't referenced
- **Hide Empty Labels**: Hide labels with no instructions
- **Show Debug Info (.loc)**: Display debug location information
- **Show Comments**: Display assembly comments
- **Demangle Identifiers**: Convert mangled C++ names to readable form

**Tools Menu:**
- **Docker Compiler Manager**: Open the compiler image management dialog
- **Insert Snippet**: Insert code snippets from the snippet library
- **Theme**: Change the application color theme

## Source Editor Keyboard Shortcuts

- **Ctrl+N**: New file
- **Ctrl+O**: Open file
- **Ctrl+S**: Save file
- **Up/Down Arrows**: Navigate lines
- **Tab**: Insert tab character (configurable width)
- **Backspace/Delete**: Delete characters

## Code Highlighting

The source editor provides syntax highlighting for C++ code, including:
- Keywords (if, else, for, while, return, etc.)
- Types (int, float, double, void, etc.)
- Strings and character literals
- Comments (single-line and multi-line)
- Preprocessor directives

## Theming

OCTOS includes multiple color themes:
- **Dark Theme** (default): Dark background with light text

Themes can be changed from the Tools > Theme menu.

## Status Indicators

- **Compiler Output**: Each pane displays the compilation result or error messages
- **Compilation Status**: Pane shows "Compiling..." during compilation
- **Error Display**: Compilation errors are shown in the pane with error formatting

## Drag and Drop

Source code files can be dragged and dropped onto the OCTOS window to load them into the editor.
