# Environment Variables

| Variable | Used by | Meaning |
| --- | --- | --- |
| `OCTOS_SETTINGS_DIR` | the application and all tests | store settings in an INI file in this directory instead of the platform store |
| `QT_QPA_PLATFORM=offscreen` | tests, CI | run the GUI without a display (CTest sets it for GUI tests) |
| `OCTOS_SCREENSHOT_DIR` | `test_main_window`, `test_project_ui` | write per-theme and feature screenshots there |
| `OCTOS_DUMP=1` | `test_asm_parser` | print the parsed corpus (functions, blocks, edges) |
| `PATH` | Docker manager/backend | `docker` is looked up in `PATH`; tests put a scripted fake `docker` first |
| `TSAN_OPTIONS`, `ASAN_OPTIONS`, `UBSAN_OPTIONS` | sanitizer builds | standard sanitizer settings (no OCTOS-specific suppressions are required) |
