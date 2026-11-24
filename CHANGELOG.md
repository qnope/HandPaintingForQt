# Changelog

All notable changes to Hand Painting for Qt are written here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html): before 1.0, a minor version
(0.1 to 0.2) may change the API and the ABI, a patch version (0.1.0 to 0.1.1) does not.

## [Unreleased]

## [0.1.0] - 2026-11-24

The first release.

### Added

- `hpq::apply_theme` and `hpq::remove_theme`: give a window and all its widgets a hand-drawn
  look, à la Rough.js, and take it away. Painting goes through `HandPaintEffect` and
  `HandPaintStyle`, so the widgets keep their behavior and the code of the application does
  not change.
- `hpq::WindowTheming`: an event filter that gives the theme to the menus, dialogs and tooltips
  of the application, which are windows of their own.
- Nine themes, each with a solid or a hatched fill (`FillStyle::Solid`, `FillStyle::Hachure`):
  - `sketch_theme`: graphite pencil on paper;
  - `china_ink_theme`: a brush loaded with ink on rice paper;
  - `slate_theme`: white and colored chalk on a black slate;
  - `notebook_theme`: a fountain pen on the Seyès ruled paper of a French school notebook;
  - `velleda_theme`: dry-erase markers on a white board;
  - `watercolor_theme`: washes and brush lines on a grainy paper;
  - `blueprint_theme`: white ink on blue paper with a grid;
  - `invisible_ink_theme`: text that shows only where it is selected;
  - `highlighter_theme`: fluorescent highlighters over notes in black pen.
- `hpq::Theme`, to make a theme of your own: palette, font, roughness (`RoughOptions`), fill
  style, medium (pencil, chalk, ink, watercolor), ink of the text and marker of the labels.
- `hpq::GeometryEngine` and `hpq::RoughGeometryEngine`: how shapes are made rough. A shape
  keeps its wobble between repaints and when a view scrolls; an engine with a fixed base seed
  draws the same window every time.
- `hpq::HandPaintDevice` and `hpq::HandPaintEngine`: hand-paint what a `QPainter` draws, for
  custom painting outside of the widgets.
- `hpq::PaintCache`: keeps the rough paths and the washes of the shapes between repaints.
- Fonts embedded in the library (SIL Open Font License): Architects Daughter, Playwrite FR
  Moderne and Kaushan Script.
- `hpq/Version.h` (`HPQ_VERSION_MAJOR`, `HPQ_VERSION_MINOR`, `HPQ_VERSION_PATCH`,
  `HPQ_VERSION_STRING`) and `hpq::version()`.
- A shared library on Windows (MSVC), Linux and macOS (gcc, clang), a static one with Qt for
  WebAssembly. The example application runs in the browser:
  https://qnope.github.io/HandPaintingForQt/
- CMake integration: the target `hpq::hpq` with `find_package(HandPaintingForQt)` after
  `cmake --install`, with `FetchContent` or with `add_subdirectory`. In a subproject, only the
  library is built. Options `HPQ_BUILD_TESTS`, `HPQ_BUILD_EXAMPLES`, `HPQ_INSTALL` and
  `HPQ_BUILD_BENCH`.
- `Examples/Integration`: a stand-alone application that uses hpq in each of these three ways,
  built and run by the CI on every platform.

[Unreleased]: https://github.com/qnope/HandPaintingForQt/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/qnope/HandPaintingForQt/releases/tag/v0.1.0
