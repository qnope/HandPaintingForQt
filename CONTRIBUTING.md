# Working on Hand Painting for Qt

How to build the project, run its tests, build the WebAssembly site, render the images of the
README and publish a version. To use hpq in an application, see the [README](README.md).

- [Build and run the tests](#build-and-run-the-tests)
- [Shared library](#shared-library)
- [WebAssembly](#webassembly)
- [Images of the README](#images-of-the-readme)
- [Benchmark](#benchmark)
- [Changelog](#changelog)
- [Publish a version](#publish-a-version)

## Build and run the tests

You need Qt 6 with the Widgets module, a C++17 compiler, CMake 3.16 or newer, and vcpkg next to
the repository (`../vcpkg`) for GoogleTest:

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<qt>
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Or define the toolchain of vcpkg and the path of Qt in a local `CMakeUserPresets.json`, which
git ignores, and use the presets:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The local presets use the Ninja generator (`build/debug`, Debug). The `release` preset builds
in `build/release`: `cmake --preset release`, `cmake --build --preset release`,
`ctest --preset release`. Without presets, add `-G Ninja` to the first command.

The tests run without a screen with `QT_QPA_PLATFORM=offscreen`, as the CI runs them on
Windows (MSVC), macOS (clang) and Linux (gcc). The CI also installs hpq and builds
[`Examples/Integration`](Examples/Integration) against it with `find_package`, `FetchContent`
and `add_subdirectory`.

## Shared library

`hpq` is built as a shared library (`hpq.dll`, `libhpq.so`, `libhpq.dylib`). Only what its
headers mark with `HPQ_EXPORT` (`hpq/Export.h`) is visible outside it: its public classes and
functions.

On Linux and macOS the executables of the build find the library by RPATH. Windows has no
RPATH, so `hpq.dll` and the executables are all built in `build/bin` there.

With Qt for WebAssembly, which is static, `hpq` stays a static library.

## WebAssembly

The example application also builds with Qt for WebAssembly. With the Emscripten release that
matches the Qt version activated (3.1.56 for Qt 6.8):

```sh
cmake -S . -B build/wasm -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=<qt>/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake \
      -DQT_HOST_PATH=<qt>/gcc_64 -DHPQ_BUILD_TESTS=OFF
cmake --build build/wasm
```

`build/wasm/Examples` then holds the site: `index.html` (from `Examples/web`),
`HandPaintingExample.js`, `HandPaintingExample.wasm`, `qtloader.js` and `fonts`. Serve it over
HTTP, for instance with `python3 -m http.server -d build/wasm/Examples`.

`HPQ_BUILD_TESTS=OFF` leaves the tests, and so GoogleTest, out of the build.

The example is linked with asyncify (`-sASYNCIFY -Os`, in `Examples/CMakeLists.txt`), so that
calls that reenter the event loop, such as `QDialog::exec()` or `QMenu::exec()`, return as they
do on the desktop. It makes the `.wasm` file larger. `open()` and `show()` need no asyncify.

On a phone, Qt opens the virtual keyboard by giving the focus to a text field that it keeps
hidden in the page. `index.html` keeps the browser from moving this focus after a tap, which
closed the keyboard at once in Safari on iOS, and switches off the corrections of the keyboard
in this field, which Qt does not follow. A double tap on a cell of the table or of the tree
opens its editor without a tap on the editor itself, so Qt does not ask for the keyboard:
`ItemEditorInputPanel` (`Examples/src`) asks for it when the tap is released.

The `Pages` workflow (`.github/workflows/pages.yml`) does this build. It checks with
`.github/scripts/WasmSmokeTest.cjs` that the application starts in a browser, and with
`.github/scripts/WasmMobileInputTest.cjs` that text can be typed in the line edit and in a cell
of the table in a browser that emulates a phone. On `main`, it deploys the site to GitHub
Pages, with the teaser video of the README (`docs/teaser/teaser.mp4`). The repository setting
Pages > Build and deployment > Source must be "GitHub Actions".

## Images of the README

The images of the README are drawn by hpq, by three tools built with the examples
(`HPQ_BUILD_EXAMPLES`). They run without a screen. The themes have a fixed seed, so the same
code draws the same images.

| Images | Tool | Command |
| --- | --- | --- |
| The logo (`docs/logo`) | [`tools/logo`](tools/logo/Logo.cpp) | `QT_QPA_PLATFORM=offscreen build/tools/logo/HpqLogo docs/logo` |
| The window in each theme (`docs/images`) | [`tools/screenshots`](tools/screenshots/ThemeScreenshots.cpp) | `QT_QPA_PLATFORM=offscreen build/tools/screenshots/HpqThemeScreenshots docs/images` |
| The teaser (`docs/teaser`) | [`tools/teaser`](tools/teaser/TeaserVideo.cpp) | `QT_QPA_PLATFORM=offscreen build/tools/teaser/HpqTeaserVideo docs/teaser` |

The logo is `logo.png` for a light page, `logo-dark.png` for a dark one and `icon.png`, the
mark alone in a square. Its drawing is in [`tools/logo/LogoDrawing.h`](tools/logo/LogoDrawing.h):
the teaser, which opens and ends with the logo, draws it with the same code.

The screenshots are the window of [`Examples/Integration`](Examples/Integration). They always
show what hpq draws now: a pull request that changes the drawing renders them again, and only
the expected images change. `plain.png`, the window without a theme, has the style and the font
of the machine that renders it. The CI renders the screenshots on Linux for each pull request
(artifact `theme-screenshots`).

The teaser needs ffmpeg in `PATH`. The tool writes `teaser.mp4` and its preview `teaser.gif`,
and the `Pages` workflow publishes `teaser.mp4` with the site. The teaser weighs several
megabytes: it is rendered again before a release, when what hpq draws has changed, and not in
each pull request.

## Benchmark

`HPQ_BUILD_BENCH=ON` builds `HpqRepaintBench`, which measures the repaints of the example
window under each theme. See [`bench/README.md`](bench/README.md).

## Changelog

Each notable change goes in the `[Unreleased]` section of [CHANGELOG.md](CHANGELOG.md).

## Publish a version

1. Set the version in `project()` (`CMakeLists.txt`) and in `vcpkg.json`.
2. Move the changes of `[Unreleased]` to the section of the version in `CHANGELOG.md`. The
   `HpqVersionsAgree` test checks that the three agree.
3. Merge, then push the tag `v<version>`, or run the `Release` workflow by hand.

The `Release` workflow (`.github/workflows/release.yml`) creates the GitHub release with the
notes of the changelog.
