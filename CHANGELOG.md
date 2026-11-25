# Changelog

All notable changes to Hand Painting for Qt are written here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html): before 1.0, a minor version
(0.1 to 0.2) may change the API and the ABI, a patch version (0.1.0 to 0.1.1) does not.

## [Unreleased]

### Added

- A teaser video at the top of the README: a Qt window drawn by hand in the nine themes while
  a pointer uses its widgets. `tools/teaser` (`HpqTeaserVideo`) renders it frame by frame,
  offscreen, with ffmpeg, and the `Pages` workflow publishes it with the site.

### Changed

- A repaint of a part of a window, such as a hovered button or the blinking cursor of a field,
  draws only the shapes, texts and images that reach that part: the others are not made rough
  nor sent to the paint engine of Qt. Such a repaint is 2 to 3 times faster; the drawing is the
  same.
- When Qt repaints a part of a window, `HandPaintEffect` renders only the widgets close to that
  part, instead of the whole window: a hovered button is repainted in 0.1 to 0.3 ms instead of
  0.7 to 1 ms. A widget without children, the viewport of a view and a widget that fills its
  background with a color of its own are painted whole; a widget of the application that holds
  other widgets and draws something else than a frame or a plain background must draw the same
  shapes whatever part of it is repainted.

### Fixed

- The opacity of a `QPainter` (`setOpacity`) reaches the hand-painted drawing: a shape, a
  line, a text or an image drawn at 30 % no longer comes out opaque.
- A pen that is not cosmetic is as wide as the transform of the painter makes it, as Qt draws
  it: a pen of 2 pixels under `scale(3, 3)` is 6 pixels wide, no longer 2.
- A gradient follows the transform and the brush origin of the painter: a widget that is not
  at the origin of its window, or a translated painter, no longer gets its gradient at the
  wrong place. The inactive tabs, the progress bars and the check boxes of Fusion get back
  their light gradient.
- A theme made before the `QApplication` no longer crashes in `QFontDatabase`: it warns and has
  no font family, and the themes made once the application exists get their font.
- A painter with the `QPainter::Antialiasing` hint gets a smooth drawing, as Qt draws it: the
  hint no longer stops at the hand-painted engine. The shapes of the example application, and
  in Fusion the fields, the check boxes, the radio buttons and the slider, are smoothed.
- A bold or an italic text stays bold or italic in the font of the theme: for example, the
  header of a view writes in bold the columns of the selected row, as Qt does.
- A view that scrolls no longer leaves traces. Qt moves the pixels it already painted and
  repaints only what comes into view, but the slate and the paper stay where they are and a
  line along the scroll keeps its wobble: the slate and the watercolor showed seams in the
  ground, the other themes bits of lines. `HandPaintEffect` now repaints a scroll area whole,
  with what its scroll bars draw past its edges, when it scrolls or when a scroll bar changes,
  shows or hides.

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
