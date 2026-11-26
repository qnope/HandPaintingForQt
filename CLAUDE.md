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

WebAssembly : `-DHPQ_BUILD_TESTS=OFF` (pas de GoogleTest), page dans `Examples/web`, exemple lié avec asyncify (`-sASYNCIFY -Os`, pour `exec()`), déploiement GitHub Pages par `.github/workflows/pages.yml` (voir CONTRIBUTING.md).

## Directories

1. Examples
2. hpq/src
3. hpq/include/hpq
4. Examples/Integration : application autonome qui consomme hpq (find_package, FetchContent, add_subdirectory), construite et lancée par la CI
5. tools/screenshots : `HpqThemeScreenshots` rend la fenêtre de `Examples/Integration` dans chaque thème, et sans thème (`plain.png`) ; images du README dans `docs/images` (`QT_QPA_PLATFORM=offscreen build/tools/screenshots/HpqThemeScreenshots docs/images`)
6. cmake : `HandPaintingForQtConfig.cmake.in` (paquet CMake) et `CheckVersions.cmake`
7. tools/teaser : `HpqTeaserVideo` rend le teaser du README avec ffmpeg, `docs/teaser/teaser.mp4` et son aperçu `teaser.gif` (`QT_QPA_PLATFORM=offscreen build/tools/teaser/HpqTeaserVideo docs/teaser`) ; le workflow `Pages` publie `teaser.mp4` avec le site
8. tools/logo : `HpqLogo` dessine le logo du README avec hpq, `docs/logo/logo.png` (crayon, page claire), `logo-dark.png` (craie, page sombre) et `icon.png` (`QT_QPA_PLATFORM=offscreen build/tools/logo/HpqLogo docs/logo`) ; le dessin est dans `tools/logo/LogoDrawing.h`, que `tools/teaser` inclut pour montrer le même logo

## Consideration for CMake

1. Using vcpkg: directory is at `../vcpkg`
2. Never use global variable => use targets
3. `hpq` est une bibliothèque dynamique (`SHARED`), statique seulement en WebAssembly (Qt for WebAssembly est statique). Sous Windows, `hpq.dll` et les exécutables sont construits dans `build/bin` (`hpq_set_runtime_directory`)
4. Les avertissements (`-Wall -Wextra`, `/W4`) sont `PRIVATE` à chaque cible du projet (`hpq_set_warnings`) : une application qui lie hpq garde les siens
5. Le projet doit rester utilisable comme sous-projet (`FetchContent`, `add_subdirectory`) : en sous-projet, `HPQ_BUILD_TESTS` et `HPQ_BUILD_EXAMPLES` valent `OFF`. Les règles `install()` et l'export `hpq::hpq` (`HPQ_INSTALL`) sont dans `hpq/CMakeLists.txt`
6. Les sources d'une cible sont listées dans son `CMakeLists.txt`, une par ligne et par ordre alphabétique (`hpq/CMakeLists.txt` : en-têtes publics puis sources). Un exécutable du projet se crée avec `hpq_add_executable(nom sources…)`, qui lui donne moc, les avertissements et le répertoire de sortie

## Consideration for code

* namespace: `hpq`
* Prefer values over pointers
* File names: `PascalCase`
* Introduce FwdDecls.h
* Public classes and free functions of `hpq/include/hpq` carry `HPQ_EXPORT` (`hpq/Export.h`): without it, they are not visible outside the shared library
* `Theme` and `RoughOptions` are structs (`hpq/Theme.h`): fields, no accessor; the seed of the hand is `RoughOptions::seed`. The nine themes are in `hpq/Themes.h`, the rough geometry is four free functions (`hpq/RoughGeometry.h`)
* `hpq/Hpq.h` includes every public header: a new one is added to it
* Type Names: `PascalCase`
* variable: `snake_case`
* function_names: `snake_case`
* constant names: `PascalCase` ; no k in front of it
* attribute names: `m_snake_case` ; prefix `m_` 
* A theme gives its colors by role with `PaletteColors` and `make_palette` (`hpq/src/ThemePalette.h`); the embedded fonts come from `hpq/src/Fonts.h` (one Qt resource, `hpq_fonts`)

## Images du README

Les images du README (`docs/images`) montrent toujours le rendu actuel : un changement qui modifie ce que dessine hpq les régénère dans la même pull request (`QT_QPA_PLATFORM=offscreen build/tools/screenshots/HpqThemeScreenshots docs/images`), puis vérifie que seules les images attendues ont changé.

Le logo (`docs/logo`) suit la même règle, avec `HpqLogo`.

Le README est pour qui utilise hpq ; ce qui sert à travailler sur le projet (construction, WebAssembly, rendu des images, release) est dans `CONTRIBUTING.md`.

Le teaser (`docs/teaser`, plusieurs Mo) n'est pas régénéré à chaque pull request : il l'est avant une release, si ce que dessine hpq a changé depuis le dernier rendu.

## Versions et release

La version est dans `project()` (`CMakeLists.txt`), `vcpkg.json` et `CHANGELOG.md` (le test `HpqVersionsAgree` vérifie qu'ils concordent). Chaque changement notable va dans la section `[Unreleased]` de `CHANGELOG.md`. Pousser le tag `v<version>` (ou lancer le workflow `Release` à la main) crée la release GitHub avec les notes du changelog.

## Pull requests

Quand une pull request est prête (travail terminé, CI verte sur toutes les plateformes), Claude peut la fusionner lui-même, sans attendre l'accord d'Antoine.
