# Spec: cmake-improvement

Status: approved

## Goal
Bring the CMake build in line with CLAUDE.md: dependencies come from vcpkg (`../vcpkg`) through a manifest, and all settings are attached to targets instead of global variables. A `hpq` library target is added in `hpq/`.

## Behaviors
- B1: given the repository root, `vcpkg.json` exists and declares `gtest` as a dependency.
- B2: given a configure with `-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake`, the configure succeeds and prints no "CMake Warning".
- B3: given the CMake files of the project, none contains `FetchContent`; GoogleTest is found with `find_package(GTest)`.
- B4: given the CMake files of the project, none contains `CMAKE_CXX_STANDARD`, `CMAKE_AUTOMOC` or `CMAKE_AUTOUIC`; each target sets C++17 with `target_compile_features(... cxx_std_17)` and sets `AUTOMOC`/`AUTOUIC` as target properties when it needs them.
- B5: given the CMake files of the project, none contains `set(... CACHE ... FORCE)`; the build still succeeds on macOS where the AGL framework is missing.
- B6: given the targets `hpq`, `HandPaintingExampleLib`, `HandPaintingExample` and `HandPaintingExampleTests`, each one has `-Wall -Wextra` (or `/W4` on MSVC) set with `target_compile_options`, and the build produces no warning from project code.
- B7: given `hpq/CMakeLists.txt`, a target `hpq` with alias `hpq::hpq` exists, with public include directory `hpq/include`, sources in `hpq/src`, and the code in namespace `hpq`; `HandPaintingExampleLib` links `hpq::hpq`.
- B8: given the build is done, `ctest` runs and all existing tests (HandPaintingExampleTests, HandPaintingExampleExists, ConfigureNoWarning, HandPaintingExampleRunsOffscreen) pass.

## Out of scope
- Writing painting code in `hpq` (the target contains only a minimal placeholder source).
- `CMakePresets.json`; the toolchain file is given on the command line.
- Changing the behavior of the example application.

## Test command
`cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/Users/amorrier/Programmation/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=/Users/amorrier/Qt/6.8.3/macos && cmake --build build && ctest --test-dir build --output-on-failure`
