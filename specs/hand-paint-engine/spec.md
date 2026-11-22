# Spec: hand-paint-engine

Status: approved

## Goal
The `hpq` library provides a `QPaintDevice` (`hpq::HandPaintDevice`) that wraps a target `QPaintDevice` (a `QImage` in v1 tests) and a `QPaintEngine` behind it. A `QPainter` opened on the device, or `QWidget::render(&device)`, gives a hand-drawn version of the drawing on the target, close to Rough.js in spirit but written from scratch (no Rough.js code or port, so no license issue). It also provides `hpq::HandPaintEffect`, a `QGraphicsEffect` that draws its source widget through a `HandPaintDevice`, so a live, interactive widget tree looks hand-drawn. In the example application, the comboBox lets the user choose the theme of the whole main window.

The design is generic:
- A `hpq::Theme` is a plain value (background, ink and fill colors, stroke width, roughness, number of stroke passes, fill style), with three presets: Sketch, China Ink, Black slate and chalk.
- A `hpq::GeometryEngine` interface turns an ideal shape into the strokes to draw. v1 ships one engine, `hpq::RoughGeometryEngine`: each segment is drawn with random end offsets and a slight bow, in one or more passes, and curves are flattened then roughened. Other engines can be plugged in.

How the engine draws:
- It implements `drawPath` and `drawPolygon` hand-drawn. Lines, rects and ellipses reach them through Qt's default `QPaintEngine` behavior.
- Text, pixmaps and images pass through.
- The theme's stroke width replaces the painter pen width, and the target is drawn antialiased.

The library tests are a gtest target of `hpq`, registered with ctest and run offscreen. The example tests are updated for the new comboBox content (this replaces B3 and B4 of `specs/example-app/spec.md`). The code follows CLAUDE.md (namespace `hpq`, PascalCase files and types, `snake_case` functions and variables, `m_` attributes, `FwdDecls.h`, CMake targets only).

Definitions used below (image 200×200, shape near the center, `D` = theme roughness + theme stroke width, in pixels):
- *background pixel*: equal to the theme background color.
- *ink blend*: a pixel whose color lies between the theme background and the theme ink color (antialiasing and opacity allowed, tolerance 2 per channel).
- *hand-drawn outline of S*: every non-background pixel is within `D` of the ideal outline of S; every point of the ideal outline has an ink-blend pixel within `D`; and the image differs from the same shape drawn by a plain `QPainter` with the theme ink color and stroke width.

## Behaviors
- B1: given `hpq::Theme::sketch()`, the result is background `#FFFFFF`, ink `#333333`, fill `#D0D0D0`, stroke width 1.0, roughness 1.5, 2 passes, fill style Solid.
- B2: given `hpq::Theme::china_ink()`, the result is background `#F5EFE0`, ink `#000000`, fill `#808080`, stroke width 2.5, roughness 1.0, 1 pass, fill style Solid.
- B3: given `hpq::Theme::chalk()`, the result is background `#2F3B33`, ink `#F0F0E8`, fill `#A0A8A0`, stroke width 2.0, roughness 2.0, 2 passes, fill style Solid.
- B4: given a `HandPaintDevice` on a 200×200 `QImage` filled red and a theme, when a `QPainter` begins and ends on the device with no drawing, the result is that every pixel of the image is a background pixel.
- B5: given a red pen and `NoBrush`, when `drawLine(QPointF(20,100), QPointF(180,100))` is called on the device, the result is a hand-drawn outline of that segment, with only background and ink-blend pixels (no red).
- B6: given a pen and `NoBrush`, when `drawRect(QRectF(40,40,120,120))` is called, the result is a hand-drawn outline of the rectangle, and the pixel at (100,100) is a background pixel.
- B7: given a pen and `NoBrush`, when `drawEllipse(QRectF(40,40,120,120))` is called, the result is a hand-drawn outline of the ellipse.
- B8: given a pen and `NoBrush`, when `drawPolygon` is called with the triangle (100,30), (170,170), (30,170), the result is a hand-drawn outline of the triangle.
- B9: given a pen and `NoBrush`, when `drawPolyline` is called with (30,30), (100,170), (170,30), the result is a hand-drawn outline of the two open segments, and no ink is drawn between (30,30) and (170,30) beyond `D` from the two segments.
- B10: given a pen and `NoBrush`, when `drawPath` is called with a `QPainterPath` made of a line and a cubic Bézier curve, the result is a hand-drawn outline of that path.
- B11: given a blue solid brush and a pen, when `drawRect(QRectF(40,40,120,120))` is called, the result is that the pixel at (100,100) has the theme fill color, pixels farther than `D` outside the rectangle are background pixels, and the outline is hand-drawn.
- B12: given a solid brush whose color has alpha 128 and `NoPen`, when `drawRect(QRectF(40,40,120,120))` is called, the result is that the pixel at (100,100) is the theme fill color blended at about 50% (tolerance 3 per channel) over the background, and no ink-color pixel appears.
- B13: given `NoPen` and `NoBrush`, when any shape is drawn, the result is that the image stays all background pixels.
- B14: given a red pen, when `drawText(QPointF(20,100), "Hello")` is called, the result is identical to a plain `QPainter` drawing the same text with the same font on the theme background with the theme ink color (no distortion).
- B15: given a 40×40 opaque red `QImage` source, when `drawImage(QPointF(80,80), source)` is called (and the same with `drawPixmap`), the result is that the 40×40 region at (80,80) equals the source exactly.
- B16: given a painter with `translate(50,0)` (and separately `scale(2,2)`), when a rectangle is drawn, the result is a hand-drawn outline of the transformed rectangle.
- B17: given the same theme and the same seed, when the same drawing (rect + ellipse + polygon) is done twice on two devices, the result is two identical images.
- B18: given the same theme and two different seeds, when the same drawing is done, the result is two different images. A device created without a seed uses a fixed default seed, so it gives the same image every time.
- B19: given the same drawing with Sketch, China Ink and Chalk, the result is three pairwise different images, each made only of its theme's background, ink-blend and fill-blend pixels.
- B20: given a device with a user-provided `GeometryEngine` that returns the ideal shape unchanged, when a polygon is drawn with a pen and a solid brush, the result is identical to a plain `QPainter` drawing that polygon on the theme background with the theme ink color, theme stroke width and theme fill color.
- B21: given a test `QWidget` (100×80, at position (0,0)) whose `paintEvent` draws `drawRect(10,10,80,60)` with a pen and no brush, when `widget.render(&device, QPoint(50,50), QRegion(), QWidget::DrawChildren)` is called, the result is a hand-drawn outline of the rectangle (60,60,80,60) on the theme background.
- B22: given the test widget of B21 (shown offscreen) with a `HandPaintEffect` set to Sketch, when the widget is grabbed with `QWidget::grab()`, the result is a hand-drawn outline of the rectangle (10,10,80,60) on the Sketch background.
- B23: given a widget with a `HandPaintEffect`, when the effect's theme is changed from Sketch to Chalk, the result is that the next grab uses the Chalk background and ink colors.
- B24: given the main window is created, the result is that `comboBox` contains exactly 4 items, in this order: "None", "Sketch", "China Ink", "Black slate and chalk", that "None" is selected, and that the menu bar, central widget and status bar have no graphics effect (plain Qt).
- B25: given the main window is shown offscreen, when the user selects "Sketch", "China Ink" or "Black slate and chalk" in `comboBox`, the result is that the menu bar, central widget and status bar each have a `HandPaintEffect` with the matching theme, and a grab of the window shows the theme background in an empty area of the central widget.
- B26: given a theme is selected, when the user selects "None" again, the result is that the menu bar, central widget and status bar have no graphics effect, and a grab of the window equals the grab taken at start.
- B27: given "Black slate and chalk" is selected, when the user clicks `checkBox` and types "abc" in `lineEdit` (QTest), the result is that `checkBox` is toggled and `lineEdit` contains "abc" (the window stays usable).
- B28: given the application is started with `QT_QPA_PLATFORM=offscreen`, when each theme is selected in turn and then "None", the result is no crash and no Qt warning.

## Out of scope
- Stability of the drawing when a shape moves, is resized or is repainted with another transform (the random strokes may change).
- Hachure, zig-zag, cross-hatch or any fill style other than Solid (the fill style setting exists so they can be added later).
- More geometry engines than `RoughGeometryEngine` plus the test engine of B20.
- Restyling widgets with QStyle / QProxyStyle.
- Popup windows (menus opened from the menu bar, the comboBox drop-down list, tooltips) and the native window frame: they stay plain Qt.
- Performance of the live effect on large windows.
- Special handling of the window background drawn by `QWidget::render` with `DrawWindowBackground`: it goes through the engine like any filled rect.
- Hand-drawn text, pixmaps or images; keeping the painter's own colors; clipping, composition modes, gradients and pattern brushes (a brush is treated as a solid color with its alpha).
- Pixel-exact comparison across platforms (B14, B20 compare against Qt on the same machine).

## Test command
`cmake --preset debug && cmake --build --preset debug && QT_QPA_PLATFORM=offscreen ctest --preset debug --output-on-failure`
