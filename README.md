# Hand Painting For Qt

Try the example application in your browser: https://qnope.github.io/HandPaintingForQt/

## Build and test

```
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

With vcpkg and Qt, add `-DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<qt>` to the first command, or define them in a local `CMakeUserPresets.json` and use `cmake --preset debug`, `cmake --build --preset debug`, `ctest --preset debug`.

The local presets use the Ninja generator (`build/debug`, Debug). The `release` preset builds in `build/release`: `cmake --preset release`, `cmake --build --preset release`, `ctest --preset release`. Without presets, add `-G Ninja` to the first command.

## WebAssembly

The example application also builds with Qt for WebAssembly. With the Emscripten release matching the Qt version activated (3.1.56 for Qt 6.8):

```
cmake -S . -B build/wasm -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<qt>/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake -DQT_HOST_PATH=<qt>/gcc_64 -DHPQ_BUILD_TESTS=OFF
cmake --build build/wasm
```

`build/wasm/Examples` then holds the site: `index.html` (from `Examples/web`), `HandPaintingExample.js`, `HandPaintingExample.wasm`, `qtloader.js` and `fonts`. Serve it over HTTP, for instance `python3 -m http.server -d build/wasm/Examples`.

`HPQ_BUILD_TESTS=OFF` leaves the tests, and so GoogleTest, out of the build.

The `Pages` workflow (`.github/workflows/pages.yml`) does this build, checks with `.github/scripts/WasmSmokeTest.cjs` that the application starts in a browser and, on `main`, deploys the site to GitHub Pages. The repository setting Pages > Build and deployment > Source must be "GitHub Actions".
