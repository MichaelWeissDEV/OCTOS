# Compiler Registry, Verification & Images

## Three separate facts

`CompilerDefinition::verificationState()` distinguishes:

| State | Meaning |
| --- | --- |
| **Image recognized** | The image *name* matched or was inferred. Says nothing about the image's content. |
| **Plausible (not verified)** | Metadata says a complete toolchain is expected (known family, required binaries listed). Still unproven. |
| **Verified** | A probe ran *inside the image* and found every required binary and a compiler version. |
| **Verification failed** | A probe ran and failed (tool missing, no version, could not run). |

The Compiler Manager used to show "Verified" for everything that was not flagged experimental. It now
shows exactly these states.

## The probe

`DockerCompilerManager::probeToolchain(image)` (button *Verify*, never pulls):

1. `docker image inspect` → the image ID;
2. `docker run --rm --network none --cap-drop=ALL … --entrypoint sh <image> -c <script> octos_probe <flag> <binaries…>`
   — the script reports `FOUND:/MISSING:` per required binary, the architecture (`uname -m`) and the first
   line of the compiler's version command. Nothing is interpolated into the script.
3. `ToolchainProbe::parseOutput()` decides: success only if every required binary was reported *found* (a
   binary that was never reported counts as missing) and a version line exists.

Stored per image: version, architecture, image digest/ID, time, result and message. A stored result only
counts while the image ID still matches (`probeIsCurrent()`): re-pulling a tag invalidates it.
Results persist in `AppSettings` (JSON) and are imported on startup.

## Ownership of images

`CompilerRegistry::isKnownCompilerImageName()` only says "this looks like a compiler image OCTOS can use".
It is **not** ownership. `DockerCompilerManager` records images it pulled **itself** (and only if they were
not installed before) in `AppSettings`.

* The manager lists only recognised compiler images, but never assumes it owns them.
* Deleting is always an explicit user decision in the dialog. Images OCTOS did not pull show a stronger
  warning; the manager refuses to delete them without `confirmedForeign=true`, and refuses non-compiler
  images outright.
* `docker rmi` of an image used by a container fails and the message is shown.

## No blocking Docker calls in GUI paths

`listInstalledImages()` returns a cache. `refreshInstalledImagesAsync()` runs `docker images` through
`runDockerAsync()` (with timeout, one refresh in flight); a failed query emits
`installedImagesRefreshFailed` and keeps the old list — an error is never presented as "zero images".
`MainWindow` refreshes its compiler lists from `installedImagesRefreshed`.
