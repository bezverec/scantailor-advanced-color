# macOS build notes

These notes describe the local macOS build flow that worked on April 20, 2026 on Apple Silicon.

## Prerequisites

Install the required Homebrew packages:

```bash
brew install cmake qt qtwebengine boost libtiff libpng jpeg-turbo libraw little-cms2 zlib
```

Notes:

- `qt` and `qtwebengine` provide the macOS frameworks, plugins, and `macdeployqt`.
- `zlib` is keg-only, but CMake was still able to use the macOS SDK copy for linking.

## Configure

From the repository root:

```bash
/opt/homebrew/bin/cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
```

## Build

```bash
/opt/homebrew/bin/cmake --build build-macos --parallel
```

The build tree contains the native app bundle at:

```bash
build-macos/scantailor.app
```

## Build a macOS app bundle

Install the build into a staging directory:

```bash
/opt/homebrew/bin/cmake --install build-macos --prefix build-macos/stage
```

That creates a standalone app bundle at:

```bash
build-macos/stage/scantailor.app
```

The install step:

- installs the native `MACOSX_BUNDLE` target
- copies the project's compiled translation files into the bundle resources
- runs `macdeployqt` with extra QtPdf search paths so Qt frameworks and plugins are bundled locally
- clears extended attributes and applies ad-hoc signing so the bundle launches cleanly on macOS

This produces a standalone developer bundle from CMake rather than from a separate shell script.

## Launch the app bundle

```bash
open "build-macos/stage/scantailor.app"
```

## Current caveats

- The build tree now produces a native `scantailor.app`, and the install step promotes it to a standalone bundle via `macdeployqt`.
- On this machine, `macdeployqt` can still emit noisy `QtPdf` / `install_name_tool` diagnostics. The install script treats those as non-fatal only if the resulting bundle already has the expected internal framework layout.
- The bundle currently has no custom `.icns` icon configured in CMake.
- `xcodebuild` is not required for this workflow, but full Xcode may still be useful later for signing, notarization, or GUI debugging.
- The build completes with warnings, but without compile or link errors in the tested environment.
