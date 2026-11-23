# Spec: sketch-paint-engine

Status: approved

## Goal
Give a whole `QMainWindow` a hand-drawn look, close to Rough.js but written from scratch (no Rough.js code). A `hpq::HandPaintEffect` (a `QGraphicsEffect`) sends all the window's drawing through a `hpq::HandPaintEngine` (a `QPaintEngine`). This engine changes each primitive with a `hpq::GeometryEngine` and draws it as a `hpq::Theme` says. Only one theme (`SketchTheme`, solid fill, no hachure) and one geometry engine (`RoughGeometryEngine`) are made now. Both are behind interfaces, so other themes (China Ink, Black slate and chalk), fill styles (hachure) and geometry engines can be added later without changing the paint engine.

## Behaviors

### Geometry engine (`hpq::GeometryEngine` interface, `hpq::RoughGeometryEngine`)
- B1: given the line path (0,0)→(100,0), `RoughOptions{roughness = 1, stroke_count = 2, max_offset = m}` and a seed, the result of `outline()` is exactly 2 paths. Each starts within `m` of (0,0), ends within `m` of (100,0), and has no point farther than `m` from the original segment.
- B2: given the same shape, the same options and the same seed twice, the result is the same `outline()` and `fill_area()` paths, point for point.
- B3: given the same shape and options with two different seeds, the result is two different `outline()` paths.
- B4: given `roughness = 0`, the result is that every `outline()` stroke has the same geometry as the original shape (each point within 0.01 px).
- B5: given a closed shape (rectangle, ellipse, polygon, rounded rect, cubic curve path), the result is `stroke_count` outline strokes. Each one starts and ends within `max_offset` of the same point (so it looks closed), and its bounding box is inside the shape's bounding box enlarged by `max_offset`.
- B6: given a closed convex shape, the result of `fill_area()` is one closed path. Its bounding box is inside the shape's bounding box enlarged by `max_offset`, and it contains the shape's center.
- B7: given an open path (a cubic curve), the result is that `outline()` strokes are not closed: each starts near the path start and ends near the path end, within `max_offset`.

### Theme (`hpq::Theme` interface, `hpq::SketchTheme`)
- B8: given `SketchTheme`, the result is a `QPalette` where every color role (`0 .. QPalette::NColorRoles - 1`) is set explicitly for each of the 3 groups Active, Inactive and Disabled.
- B9: given the `SketchTheme` palette, the result is a light "paper" `Window` / `Base` and a dark "graphite" `WindowText` / `Text` / `ButtonText`. The contrast ratio of `WindowText` against `Window` is at least 4.5 in the Active group, and lower in the Disabled group than in the Active group.
- B10: given `SketchTheme`, the result is a font whose family is "Architects Daughter". The font is embedded in the hpq library as a Qt resource (with its SIL OFL license file), and `QFontDatabase` has the family after the theme is created.
- B11: given `SketchTheme`, the result is `fill_style() == FillStyle::Solid` and `rough_options().stroke_count == 2`.

### Paint engine (`hpq::HandPaintDevice`, `hpq::HandPaintEngine`)
- B12: given a `HandPaintDevice` that wraps a target `QPainter`, and a rectangle drawn on it with a pen and a brush, the result is that the target receives 1 fill (from `fill_area()`, in the brush color), then 2 outline strokes (in the pen color and width). None of them is the exact rectangle.
- B13: given a primitive drawn with `Qt::NoPen`, the result is that the target receives no outline stroke. Given a primitive drawn with `Qt::NoBrush`, the result is that the target receives no fill.
- B14: given lines, rectangles, ellipses, polygons (convex, odd-even, winding, polyline) and paths drawn on the device, the result is that each one goes through the `GeometryEngine` before it reaches the target.
- B15: given a `GeometryEngine` (test fake) that returns its input unchanged, and a rectangle drawn on the device, the result is that the target receives the exact rectangle as fill and as outline. This proves a new geometry engine can be plugged in without changing the paint engine.
- B16: given the same primitive drawn twice on the device (same geometry, same transform), the result is the same rough paths both times. The same primitive drawn elsewhere on the device (a view that scrolls) gets the same rough paths, moved with it (the seed comes from the primitive type and its device coordinates, counted from the top left corner of its bounding rectangle).
- B17: given a world transform (translate, scale) on the device painter, the result is that the rough shape lands at the transformed position on the target.
- B18: given a clip rectangle on the device painter, the result is that nothing is drawn on the target outside this clip.
- B19: given a text item drawn on the device in any font (for example "Helvetica" 16), the result is that the target draws the same text in the theme font ("Architects Daughter") at the same pixel size, in the pen color.
- B20: given `drawPixmap`, `drawImage` or `drawTiledPixmap` on the device, the result is that the target receives the same image at the same place, unchanged.

### Applying to a window (`hpq::HandPaintEffect`, `hpq::apply_theme`, `hpq::remove_theme`)
- B21: given a `QMainWindow` with child widgets and `apply_theme(window, theme, geometry_engine)`, the result is:
  - the window palette equals the theme palette;
  - the window font family is the theme font family;
  - the window and all its existing descendant widgets use the Fusion style;
  - the window's `graphicsEffect()` is a `HandPaintEffect`.
- B22: given a window with a `QFrame` box and the Fusion style and the Sketch palette, the result is that a grab of the window with `apply_theme` is not the same image as a grab with the same style and palette but no effect.
- B23: given a themed window shown with `QT_QPA_PLATFORM=offscreen`, the result is that it paints with no crash, no infinite recursion and no Qt warning.
- B24: given a themed window, the result is that its child widgets still get input: a mouse click on a `QCheckBox` toggles it.
- B25: given `remove_theme(window)` after `apply_theme`, the result is that the window has no graphics effect and its palette, font and style (window and descendants) are back to their values before `apply_theme`.
- B26: given `apply_theme` called twice on the same window, the result is one `HandPaintEffect`, and `remove_theme` still restores the values from before the first call.

### Example application
- B27: given the example `MainWindow` is created, the result is that `comboBox` shows "Sketch" (index 0) and the Sketch theme is applied to the window (B21 is true for it). This replaces behavior B4 of `specs/example-app` ("comboBox has no behavior").
- B28: given "China Ink" or "Black slate and chalk" is selected in `comboBox`, the result is that the theme is removed (default Qt rendering, as after `remove_theme`). Given "Sketch" is selected again, the result is that the Sketch theme is applied again.
- B29: given the example application is started with `QT_QPA_PLATFORM=offscreen`, the result is that the themed window shows and the event loop starts without crash or Qt warning.

## Out of scope
- Hachure and any other fill style except solid, and the China Ink and Black slate and chalk themes (only the interfaces are ready for them).
- Any geometry engine other than `RoughGeometryEngine`.
- Stability when moving: a moved or resized primitive may get another rough shape.
- Speed: the effect repaints the whole window at each update. No caching and no partial repaint.
- Elements that Fusion draws through its own pixmap cache (for example some button or combobox backgrounds) may stay sharp: they reach the engine as pixmaps.
- Widgets created after `apply_theme` (they get the palette and font, but not the Fusion style), dialogs and other top-level windows, native or OpenGL child widgets.
- Remapping colors that are not in the palette (hard-coded colors, such as the scene items, keep their colors and only get the rough shape).
- Changing the text layout: the substituted font is drawn at the same pixel size, even if the text is then a bit wider or narrower than its box.

## Test command
`cmake --preset debug && cmake --build --preset debug && QT_QPA_PLATFORM=offscreen ctest --preset debug`
