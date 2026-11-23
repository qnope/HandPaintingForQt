# Hand Painting For Qt

Simple library allowing to perform custom painting à la Rough.js with Qt widgets.

## Themes

1. Sketch
2. China Ink
3. Black slate and chalk
4. School notebook
5. Velleda
6. Watercolor
7. Blueprint
8. Invisible ink
9. Highlighter

## Build Targets
1. Windows: MSVC
2. Linux, MacOS : gcc, clang
3. Web Assembly

## Technologies

1. Qt
2. C++17
3. CMake
4. Google Tests

## How to build / tests

Toolchain vcpkg et chemin de Qt sont dans `CMakeUserPresets.json` (preset `debug`, fichier local ignoré par git).

```
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Sans le preset : `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<qt>`, puis `cmake --build build --config Debug` et `ctest --test-dir build -C Debug --output-on-failure`.

WebAssembly : `-DHPQ_BUILD_TESTS=OFF` (pas de GoogleTest), page dans `Examples/web`, exemple lié avec asyncify (`-sASYNCIFY -Os`, pour `exec()`), déploiement GitHub Pages par `.github/workflows/pages.yml` (voir README).

## Directories

1. Examples
2. hpq/src
3. hpq/include/hpq

## Consideration for CMake

1. Using vcpkg: directory is at `../vcpkg`
2. Never use global variable => use targets

## Consideration for code

* namespace: `hpq`
* Prefer values over pointers
* File names: `PascalCase`
* Introduce FwdDecls.h
* Type Names: `PascalCase`
* variable: `snake_case`
* function_names: `snake_case`
* constant names: `PascalCase` ; no k in front of it
* attribute names: `m_snake_case` ; prefix `m_` 

## Pull requests

Quand une pull request est prête (travail terminé, CI verte sur toutes les plateformes), Claude peut la fusionner lui-même, sans attendre l'accord d'Antoine.
