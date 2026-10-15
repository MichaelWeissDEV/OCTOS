# Keyboard Shortcuts

Shortcuts are written for Windows/Linux; on macOS **Ctrl** is the **⌘ Command** key and **Alt** is **⌥ Option**.

## Application

| Shortcut | Action |
| --- | --- |
| `F5` or `Ctrl+Return` | **Compile Now** (Tools menu / toolbar button) — also when auto-compile is off |
| `Ctrl+N` | New file (asks to close an open project first) |
| `Ctrl+O` | Open file (asks to close an open project first) |
| `Ctrl+S` | Save file; **in project mode: save the current file into the project** |
| `Ctrl+Shift+S` | Save As (platform *Save As*); in project mode: save the project under a new name |
| `Ctrl+Alt+S` | **Save Project** |
| `Ctrl+Q` | Quit |

## Editor

| Shortcut | Action |
| --- | --- |
| `Tab` | indent to the next 4-column stop (or indent the selected lines) |
| `Shift+Tab` | unindent the line / selected lines |
| `Enter` | new line with the same indentation (one level deeper after `{` or `:`) |
| `Ctrl+F` | open the find bar (platform *Find* key) |
| `Ctrl+H` | open find & replace (platform *Replace* key; may differ on some platforms) |
| `Enter` / `Shift+Enter` in the find field | next / previous match |
| `F3` / `Shift+F3` (also `Ctrl+G` / `Ctrl+Shift+G`) | next / previous match |
| `Esc` | close the find bar |
| `Ctrl+Z`, `Ctrl+Y` / `Ctrl+Shift+Z`, `Ctrl+C`, `Ctrl+V`, `Ctrl+X`, `Ctrl+A` | standard editing |

## Diff viewer

| Shortcut | Action |
| --- | --- |
| `Ctrl+C` | copy the selected rows (tab-separated) |
| `Enter` in the search field | next match |

## Notes

* All menu shortcuts are unique; a test fails if two actions ever share a key (Qt would then disable both).
* The *Replace* and *Find* keys come from the platform theme (`QKeySequence::Replace`/`Find`); the table lists
  the common values.
