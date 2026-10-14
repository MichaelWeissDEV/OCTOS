# Side-by-Side Compiler Comparison

OCTOS allows comparing different compilers, compiler versions, or optimization levels simultaneously.

## Adding Comparison Panes

Click the **+ Compare** button on the source code toolbar to add an additional compiler output pane.

Each pane operates independently:
- **Language**: Select the target programming language.
- **Compiler**: Select from installed Docker compiler images.
- **Syntax**: Choose between Intel and AT&T syntax flavors (for supported languages).
- **Flags**: Custom flags (e.g. `-O3 -march=native` vs `-O0 -g`).

## Closing Panes

- Click the **✕** button on any pane's header to close it.
- Click **Close Others** on the source toolbar to close all panes except the primary one.
- Closing a pane automatically cancels all active and pending compilation jobs associated with that pane.
