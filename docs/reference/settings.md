# Settings Reference

OCTOS keeps its settings in Qt's `QSettings` store (organisation `OCTOS`, application `OCTOS`):

| Platform | Location |
| --- | --- |
| macOS | `~/Library/Preferences/com.octos.OCTOS.plist` |
| Linux | `~/.config/OCTOS/OCTOS.conf` |
| Windows | registry `HKEY_CURRENT_USER\Software\OCTOS\OCTOS` |

Set the environment variable **`OCTOS_SETTINGS_DIR`** to a directory to store everything in an INI file there
instead (portable setups; the test suite uses it so that tests never touch your real settings).

## Keys

| Key | Type / default | Range & notes |
| --- | --- | --- |
| `Editor/AutoCompile` | bool, `true` | compile automatically after edits (Tools menu) |
| `Editor/DebounceIntervalMs` | int, `300` | clamped to 50 … 5000 ms |
| `Backend/MaxParallelJobs` | int, max(2, CPU threads) | clamped to 1 … 16; concurrently running compilations |
| `UI/ThemeIndex` | int, `0` | 0 Dark, 1 Light, 2 Solarized Dark, 3 Solarized Light, 4 Dracula, 5 Nord |
| `Docker/MemoryLimit` | string, `512m` | `^[1-9]\d*[kKmMgG]?$`; invalid values fall back to the default |
| `Docker/CpuLimit` | string, `1.5` | > 0 and ≤ 64 |
| `Docker/PidsLimit` | int, `128` | 16 … 4096 |
| `Docker/OctosPulledImages` | string list | images OCTOS downloaded itself (deletion warning logic) |
| `Docker/ToolchainProbes` | JSON | stored verification results per image |
| `Profiles/Json` | JSON array | user-level compiler profiles |
| `Layout/geometry`, `Layout/state`, `Layout/splitter`, `Layout/outputSplitter`, `Layout/defaultState` | binary | window layout |
| `General/LastDirectory` | string | start directory of file dialogs |
| `RecentFiles/*` | | recent files list |

Each Docker limit is validated **individually**: one invalid value falls back to *its own* default and does not
reset the others.

## Limits that are not (yet) user settings

These come from `DockerExecutionConfig` defaults:

| Limit | Default |
| --- | --- |
| compile timeout | 30 s (1 … 300 s allowed) |
| max source (main + extra files) | 1 MB |
| max output per stream (stdout / stderr) | 5 MB |
| max size of one artifact | 10 MB |
| max total artifact size | 20 MB (also the per-file write limit inside the container) |
| `/tmp` tmpfs size | 64 MB |
| docker start timeout / cleanup timeout | 5 s / 8 s |
| `--network none`, `--cap-drop=ALL`, `no-new-privileges`, `--ipc none`, swap disabled | always on |
