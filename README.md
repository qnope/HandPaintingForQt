# Hand Painting For Qt

## Build and test

```
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

With vcpkg and Qt, add `-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<qt>` to the first command, or define them in a local `CMakeUserPresets.json` and use `cmake --preset debug`, `cmake --build --preset debug`, `ctest --preset debug`.

The local presets use the Ninja generator (`build/debug`, Debug). The `release` preset builds in `build/release`: `cmake --preset release`, `cmake --build --preset release`, `ctest --preset release`. Without presets, add `-G Ninja` to the first command.

## Requirements

Qt 6.4 or later (the CI builds with Qt 6.4 and Qt 6.8), a C++17 compiler, CMake 3.16 or later and GoogleTest.

On Ubuntu 24.04 the distribution packages are enough, without vcpkg nor a Qt installer:

```
scripts/InstallUbuntuDependencies.sh
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
QT_QPA_PLATFORM=offscreen ctest --test-dir build/debug --output-on-failure
```

In a container without a user session, also give the tests a runtime directory, e.g. `XDG_RUNTIME_DIR=$(mktemp -d)`: otherwise Qt warns that it is not set and the tests that expect no warning fail.
