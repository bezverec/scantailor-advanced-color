# ScanTailor Advanced Color

The ScanTailor version that merges the features of the `ScanTailor Featured`, `ScanTailor Enhanced`, `ScanTailor Universal` and `ScanTailor Advanced` forks,
developed by `OpenAI Codex` with human supervision.

This fork is also a good fit for preservation-oriented workflows where the operator needs only minimal interventions
such as cropping, rotating, deskewing, page splitting, and light cleanup, while preserving the source image
characteristics as much as possible.

### Screenshot
<img width="2560" height="1392" alt="scantailor_advanced_color_1 1 2" src="https://github.com/user-attachments/assets/cd272470-cad1-4eae-af01-30c44896315c" />

## Description

ScanTailor is an interactive post-processing tool for scanned documents. 

## Preservation-oriented workflow

Besides the classic "prepare pages for reading / printing" use case, this fork can also be used for
preservation-master style processing where only the page geometry should change and the image data should stay as close
to the source as possible.

In particular, the current codebase contains support for:

- preserving higher bit-depth image paths where Qt / libtiff make that possible
- preserving and propagating ICC profile / color space metadata through the processing pipeline
- writing TIFF output with embedded ICC profiles
- using Little CMS 2 (`lcms2`) helpers for fallback ICC profile generation when needed

This is especially relevant for TIFF / TIF collections where the user wants to crop, rotate, deskew, or split pages
without unnecessarily flattening the source into a lower-fidelity derivative.

## Why "Color"

The `Color` suffix was added to make the purpose of this branch explicit.

The main reason is the addition of **Little CMS 2 (`lcms2`)** and the related color-management work around it:

- detecting and preserving ICC profiles from TIFF / TIF input
- propagating color space information through the processing pipeline
- embedding ICC profiles back into TIFF output
- generating fallback ICC profiles when the source image has no explicit profile but a color-managed output is still desirable

In other words, this is not just "Advanced with a few extra tweaks". It is an Advanced branch that explicitly cares about
color fidelity, bit depth, and preservation-oriented output behavior.

## Building

Recommended build paths:

- Windows native with MSVC + `vcpkg` and the repository `vcpkg.json` manifest
- Linux native with distro packages
- Linux to Windows cross-compile with MXE using `build-windows.sh`

The old `../libs` layout from `scantailor-libs-build` is still supported, but for new Windows setups the native
`vcpkg` flow below is the easiest one to reproduce.

See [Building (detailed)](#building-detailed) for the complete instructions.

## Kudos

The original ScanTailor and ScanTailor Advanced work remains credited to its historical authors and contributors. Kudos to all the [forkers](https://github.com/ScanTailor-Advanced/scantailor-advanced/forks?include=active&page=1&period=1y&sort_by=stargazer_counts) keeping the project alive and diverse, namely [pablogventura](https://github.com/pablogventura/scantailor-advanced) and [vigri](https://github.com/vigri/scantailor-advanced), also check out their awesome website [scantailor.net](https://scantailor.net/). 

For this `ScanTailor Advanced Color` fork, the preservation-color changes, LCMS2 integration, Windows build refresh,
and the current README / build documentation were implemented with **OpenAI Codex**, in collaboration with the
repository owner.

## License

This software is licensed under GNU GPLv3, you can read more about it on our [LICENSE](/LICENSE) file.

## Building (detailed)

### Build overview

Supported and practical build paths:

- Windows native: MSVC + CMake + `vcpkg` using the checked-in `vcpkg.json`
- Linux native: GCC / Clang + CMake + distro packages
- Linux to Windows cross-compile: MXE using `build-windows.sh`

General notes:

- Out-of-source builds are required.
- `BUILD_TESTS` defaults to `ON`. For offline, minimal, or packaging-oriented builds you can use `-DBUILD_TESTS=OFF`.
- `ENABLE_LCMS2` defaults to `ON`. If `lcms2` is missing, the project still configures, but TIFF ICC fallback profile helpers are disabled.

### Building on Windows (native, recommended)

This is the easiest way to build the current preservation-focused branch on Windows, including TIFF, ICC, and LCMS2
support.

Prerequisites:

1. Visual Studio with **Desktop development with C++** and a recent Windows SDK
2. CMake (the Visual Studio bundled CMake is fine)
3. `vcpkg` (the Visual Studio bundled `vcpkg` is fine)

The repository includes a `vcpkg.json` manifest with the required dependencies:

- Boost
- libjpeg-turbo
- libpng
- libtiff
- zlib
- Qt6 (`qtbase`, `qtsvg`, `qttools`)
- Little CMS (`lcms`, providing `lcms2`)

From an **x64 Visual Studio Developer Command Prompt**:

```bat
cd path\to\scantailor-advanced

set VCPKG_ROOT=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\vcpkg

"%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows

cmake -S . -B build-vcpkg -G "NMake Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DBUILD_TESTS=OFF ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build-vcpkg
```

Notes:

- Adjust `%VCPKG_ROOT%` if you use a different Visual Studio version / edition or a standalone `vcpkg`.
- Prefer `Release` builds on Windows. If a build directory is already cached as `Debug`, recreate or reconfigure it as `Release` before running the deploy step.
- The executable is written to `build-vcpkg\scantailor.exe`.
- The post-build step runs `windeployqt` to copy the required Qt runtime files next to the executable.

Alternative Windows dependency layout:

- The project still supports the older `../libs` layout used by `scantailor-libs-build`.
- If you already maintain that layout, you can continue using it and point the CMake cache variables (`JPEG_INSTALL_PREFIX`, `PNG_INSTALL_PREFIX`, `TIFF_INSTALL_PREFIX`, `QT_INSTALL_PREFIX`, `BOOST_ROOT`, and so on) at your dependency prefixes as needed.

### Building on Linux (Ubuntu / Debian)

The example below uses Qt5 package names because they are widely available on Ubuntu / Debian. The project also builds
with Qt6 if the matching development packages are installed.

Install the required dependencies:

```bash
sudo apt install build-essential cmake \
  qt5-qmake qtbase5-dev libqt5svg5-dev qttools5-dev \
  libboost-dev libboost-test-dev \
  libjpeg-dev libpng-dev libtiff-dev zlib1g-dev liblcms2-dev
```

Configure and build:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$(nproc)"
```

For offline or minimal chroot builds where unit tests cannot be built, use:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
```

The executable will be in the build directory. To install system-wide:

```bash
sudo make install
```

To build a `.deb` package (with application icon and menu entry):

```bash
./build-deb.sh
```

This produces `scantailor-advanced_<version>_<arch>.deb` in the project root. Install with:

```bash
sudo dpkg -i scantailor-advanced_*.deb
```

### Building a Windows `.exe` from Linux (cross-compile)

You can cross-compile the Windows executable from Linux using [MXE](https://mxe.cc/) (M Cross Environment).

One-time MXE setup (builds Qt5 and dependencies; can take 1–2 hours):

```bash
git clone https://github.com/mxe/mxe.git ~/mxe
cd ~/mxe
make MXE_TARGETS=x86_64-w64-mingw32.static qt5 jpeg libpng tiff zlib boost lcms
```

Then from the ScanTailor Advanced source directory:

```bash
MXE_DIR=~/mxe ./build-windows.sh
```

This produces `build-win-static/scantailor.exe`.

For a shared build (smaller `.exe`, external DLLs required):

```bash
MXE_DIR=~/mxe ./build-windows.sh shared
```

### Platform-specific notes

**Windows – large JPEG/PNG (issue #20):** If large-format JPEG or PNG files fail to load on Windows, ensure the build
uses **libjpeg-turbo** (or libjpeg 9+) and a recent libpng.

**Linux – Wayland (issue #97):** If you see rendering issues (blank or corrupted windows) when running under Wayland,
try starting the application with `QT_QPA_PLATFORM=xcb` to use the X11 compatibility layer.

**Linux – Flatpak / Flathub (issue #105):** A Flatpak manifest is provided in `flatpak/org.scantailor.Advanced.json`.
To build locally:

```bash
flatpak-builder --user --force-clean build flatpak/org.scantailor.Advanced.json
```

This requires `flatpak` and `flatpak-builder`. To publish on Flathub, use a distinct application ID such as
`org.scantailor.Advanced` so it does not conflict with the original ScanTailor package.
