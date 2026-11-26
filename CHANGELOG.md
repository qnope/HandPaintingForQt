# Changelog

All notable changes to Hand Painting for Qt are written here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html): before 1.0, a minor version
(0.1 to 0.2) may change the API and the ABI, a patch version (0.1.0 to 0.1.1) does not.

## [Unreleased]

The API is simpler, and not compatible with 0.1: a theme is a struct, the geometry is four
functions and the headers are 11 instead of 27. To move an application from 0.1:

| 0.1 | Now |
| --- | --- |
| `apply_theme(window, theme, geometry_engine)` | `apply_theme(window, theme)` |
| `WindowTheming(theme, geometry_engine)` | `WindowTheming(theme)` |
| `HandPaintEffect(theme, geometry_engine)` | `HandPaintEffect(theme)` |
| `HandPaintDevice(target, theme, geometry_engine, &cache)` | `HandPaintDevice(target, theme)` or `HandPaintDevice(target, theme, cache)` |
| `RoughGeometryEngine(seed)` | `theme.rough_options.seed = seed` |
| `RoughGeometryEngine()`, a random hand | `theme.rough_options.seed = std::random_device()()` |
| `engine.outline(shape, options, seed)` | `rough_outline(shape, options, seed)`, and so on |
| `theme.palette()`, `theme.font()`, ... | `theme.palette`, `theme.font`, ... |
| `Theme(palette, font, options, fill_style, ...)` | a `Theme`, then its fields |
| `<hpq/SketchTheme.h>` and the eight others | `<hpq/Themes.h>` |
| `<hpq/RoughOptions.h>`, `<hpq/FillStyle.h>`, `<hpq/Medium.h>`, `<hpq/TextInk.h>` | `<hpq/Theme.h>` |
| `<hpq/WindowTheming.h>` | `<hpq/Theming.h>` |
| `<hpq/GeometryEngine.h>`, `<hpq/RoughGeometryEngine.h>` | `<hpq/RoughGeometry.h>` |

### Added

- `AntialiasingMode` and `Theme::antialiasing_mode`: a theme smooths its lines and its
  fills `Always` (the default), `OnlyWhenAsked` by the painter with `QPainter::Antialiasing`,
  as Qt draws, or `Never`. The settings of the example application choose it.
- `RoughOptions::seed`: the hand that draws. The same seed draws the same window every time.
- `<hpq/RoughGeometry.h>`: `rough_outline`, `rough_frame_outline`, `rough_fill_area` and
  `rough_hachure`, the paths that hpq draws instead of a shape, as functions of the shape, of
  the `RoughOptions` and of the seed of the shape.
- `<hpq/Themes.h>`: the nine themes in one header.
- `<hpq/Hpq.h>` includes all of hpq.
- A teaser video at the top of the README: a Qt window drawn by hand in the nine themes while
  a pointer uses its widgets. `tools/teaser` (`HpqTeaserVideo`) renders it frame by frame,
  offscreen, with ffmpeg, and the `Pages` workflow publishes it with the site.
- A logo, drawn by hpq with a `HandPaintDevice`: `tools/logo` (`HpqLogo`) writes
  `docs/logo/logo.png` in pencil for a light page, `logo-dark.png` in chalk for a dark page and
  `icon.png`, the mark alone. The teaser opens and ends with it.
- `CONTRIBUTING.md`: how to build the project, run its tests, build the WebAssembly site,
  render the images of the README and publish a version. The README, which held it, now opens
  with the logo, a first call to `apply_theme` and the window before and after that call
  (`docs/images/plain.png`, the window without a theme, is rendered by `tools/screenshots`).
- `PaintCache::made_count()` and `HandPaintEffect::cache()`: how many times a theme had the
  paths, the pieces or the wash of a shape made, because its cache did not keep them. A test
  or a benchmark reads there whether a repaint found its shapes in the cache.

### Changed

- `Theme` is a struct: its palette, font, rough options, fill style, medium, text ink, label
  marker and antialiasing mode are fields to read and to change, in place of a constructor of
  seven arguments and of eight accessors. `FillStyle`, `Medium`, `TextInk`, `AntialiasingMode`
  and `RoughOptions` are declared with it, in `<hpq/Theme.h>`.
- `apply_theme`, `WindowTheming`, `HandPaintEffect` and `HandPaintDevice` take a theme and no
  geometry engine: the seed of the hand is in the theme. `HandPaintDevice` takes its cache by
  reference, in a constructor of its own, and keeps a copy of the theme: a theme made in the
  call (`HandPaintDevice device(target, hpq::sketch_theme())`) is fine.
- A theme draws the same window at every run, because its seed is 0 unless the application
  gives it another one. In 0.1, `RoughGeometryEngine()` took a random seed.
  `theme.rough_options.seed = std::random_device()()` gives each run its own hand, as the
  example application does.
- `WindowTheming` is declared in `<hpq/Theming.h>`, with `apply_theme` and `remove_theme`.
- Without wobble (a `roughness` or a `max_offset` of 0), the outline and the fill area of a
  shape are the shape itself: a curve stays a curve, instead of short lines at the same place.
  The themes of hpq are not concerned: their roughness goes from 0.6 to 1.
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
- By default, every line and every fill is smoothed, in every theme, whether the painter has
  the `QPainter::Antialiasing` hint or not: a rough line wobbles, and without smoothing its slopes
  turned into stairs of pixels. Smoothing costs time on the raster engine: a full repaint of
  the example window takes 1.4 to 3 times longer (Sketch: 3.3 to 6.7 ms at a device pixel
  ratio of 1, 8.9 to 24.8 ms at 2), a step of scroll 1.3 to 2.9 times longer, and a hovered
  button stays under 0.5 ms. Watercolor, which smoothed its washes already, takes as long as
  before. To keep that cost down, a line wider than a pixel is filled as an area, cut in pieces
  that the raster engine smooths quickly, and the cache keeps those pieces, up to twice 16 MB.
  `AntialiasingMode::OnlyWhenAsked` smooths only where the painter asks for it, as before.

### Removed

- `GeometryEngine` and `RoughGeometryEngine`: the four functions of `<hpq/RoughGeometry.h>`
  replace the interface and its only implementation, and no `std::shared_ptr` travels with a
  theme any more. An application can no longer give hpq a geometry of its own.
- `Theme::with_antialiasing_mode`, the constructor and the accessors of `Theme`: set the fields.
- The headers of one enum or of one theme (`<hpq/FillStyle.h>`, `<hpq/SketchTheme.h>`, ...),
  `<hpq/RoughOptions.h>` and `<hpq/WindowTheming.h>`: see the table above.
- `hpq::library_name()`, a placeholder.
- `hpq::HandPaintEngine` and `<hpq/HandPaintEngine.h>` are no longer public: the paint engine
  is a private part of `hpq::HandPaintDevice`, which is what an application gives to a
  `QPainter` to hand-paint what it draws. The drawing is the same.

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
