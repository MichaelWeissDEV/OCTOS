# Project File Format (`octos-project-v1`)

A project is a single JSON document, normally with the extension `.octos`.

## Example

```json
{
  "format_version": "octos-project-v1",
  "name": "Demo",
  "source_files": [
    { "path": "src/main.cpp", "content": "#include \"util.h\"\nint main(){return h();}\n",
      "is_header": false, "language": "cpp" },
    { "path": "src/util.h", "content": "int h();\n", "is_header": true, "language": "cpp" }
  ],
  "profiles": [
    { "id": "gcc-o3", "name": "GCC -O3", "compiler_image": "gcc:13", "compiler_version": "13",
      "language": "cpp", "optimization": "-O3", "user_flags": "-Wall",
      "target_architecture": "x86_64", "output_format": "assembly",
      "rust_crate_type": "auto", "syntax_flavor": "Intel" }
  ],
  "targets": [
    { "name": "default", "main_source_file": "src/main.cpp",
      "source_files": ["src/main.cpp", "src/util.h"], "include_paths": ["src"] }
  ],
  "configuration": {
    "active_profile_id": "gcc-o3",
    "active_target_name": "default",
    "environment_vars": { "CXXFLAGS": "-O2" }
  }
}
```

## Fields

| Field | Required | Notes |
| --- | --- | --- |
| `format_version` | recommended | `octos-project-vN`. Missing → accepted as v1 with a warning; unrecognised → refused; **newer than supported → refused** ("created by a newer OCTOS") |
| `name` | no | string |
| `source_files[]` | no | `path` (required), `content`, `is_header`, `language` (`cpp c rust csharp java python ada`; unknown → `cpp` with a warning) |
| `profiles[]` | no | `id` and `name` required; all other fields optional with defaults (`output_format` `assembly`, `syntax_flavor` `Intel`, `rust_crate_type` `auto`) |
| `targets[]` | no | `name`, `main_source_file`, `source_files`, `include_paths` |
| `configuration` | no | active profile/target and environment variables |

## Validation rules

Loading fails with a message if any of these is violated (up to five problems are listed):

* the file is larger than 256 MB, is not valid JSON, or the root is not an object; structural fields have the
  wrong type;
* more than 10 000 files, a file larger than 16 MB, or more than 128 MB of content;
* a **path** is unsafe: empty, absolute (`/…`, `C:…`, `~…`), contains `..` that escapes the project, uses `\`,
  contains control characters or NUL, is longer than 1024 characters, or is not in normalised form
  (`a//b`, `./a`);
* **duplicates**: two files with the same path, two profiles with the same `id`, two targets with the same name
  (they are no longer merged silently);
* **dangling references**: a target's main/source file that is not a project file; an `include_paths` entry that
  is not a safe relative path; an active profile/target that does not exist;
* an environment variable name that is not `[A-Za-z_][A-Za-z0-9_]*`.

## Writing

Saving validates first (an invalid project is never written) and writes through `QSaveFile`: the new content
replaces the old file in one atomic step, a failure leaves the previous file untouched.

## Versioning and migration

`Workspace::kCurrentFormatVersion` is 1. When a v2 exists, files with an older version are migrated step by
step in `Workspace::fromJson` before validation; files with a newer version are always refused.
