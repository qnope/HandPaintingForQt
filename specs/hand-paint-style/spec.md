# Spec: hand-paint-style

Status: approved

## Goal
Fusion draws several widget parts (push buttons, comboboxes, group box frames, header sections, arrows, spin boxes, sliders) into a pixmap and then copies it: the `HandPaintEngine` only receives an image and cannot make these parts rough. A new style `hpq::HandPaintStyle`, a `QProxyStyle` over Fusion, draws these parts cleanly with vector primitives only (flat fills and outlines in palette colors, no pixmap, no gradient). The pencil look stays the job of the `HandPaintEngine`. `apply_theme` installs this style in place of the plain Fusion style.

In the behaviors below, "rendered with the style" means: the widget, with the `SketchTheme` palette and `HandPaintStyle`, rendered with `QWidget::render` on the test `RecordingPaintDevice` (no `HandPaintEffect`). "No image" means the recording has no `drawPixmap`, `drawImage` and `drawTiledPixmap` call.

## Behaviors

### Style (`hpq::HandPaintStyle`)
- B1: given a `HandPaintStyle`, the result is that it is a `QProxyStyle` whose `baseStyle()` is the Fusion style (`baseStyle()->name()` is "fusion", ignoring case).
- B2: given `drawPrimitive(PE_PanelButtonCommand)` on a rectangle R with a raised (not sunken, not on) option, the result is that the recording receives a closed shape inside R, filled with a solid brush of the palette `Button` color and outlined with a pen of the palette `Dark` color, and no image.
- B3: given the same call with a sunken or on (checked) option, the result is the same shape filled with a solid brush of the palette `Midlight` color, and no image.
- B4: given `drawPrimitive` with `PE_IndicatorArrowUp`, `PE_IndicatorArrowDown`, `PE_IndicatorArrowLeft` or `PE_IndicatorArrowRight` on a rectangle R, the result is one filled triangle (a closed polygon of 3 points) inside R, in a solid brush of the palette `ButtonText` color, pointing in the asked direction (its tip is the point nearest the up, down, left or right side of R), and no image.
- B5: given `drawPrimitive(PE_FrameGroupBox)` on a rectangle R, the result is one rectangle outline inside R with a pen of the palette `Dark` color, no fill, and no image.
- B6: given a `QPushButton` with a text and a `QToolButton` with a text, rendered with the style, the result is no image, at least one shape filled with the palette `Button` color, and the text drawn.
- B7: given a non-editable `QComboBox` and an editable `QComboBox`, rendered with the style, the result is no image, a frame outlined with the palette `Dark` color, and a down arrow (B4) drawn.
- B8: given a `QGroupBox` with a title, rendered with the style, the result is no image and a frame outline with the palette `Dark` color (B5).
- B9: given a `QTableView` with a model of 3 columns and 3 rows, rendered with the style, the result is no image, and each horizontal and vertical header section is a shape filled with the palette `Button` color and outlined with the palette `Dark` color.
- B10: given a horizontal and a vertical `QScrollBar` with a range, rendered with the style, the result is no image and 2 arrows (B4) per scroll bar, in its 2 directions.
- B11: given a `QSpinBox` and a `QDoubleSpinBox`, rendered with the style, the result is no image, a frame outlined with the palette `Dark` color, and an up and a down arrow (B4).
- B12: given a horizontal and a vertical `QSlider`, rendered with the style, the result is no image, a groove and a handle both drawn as vector shapes (the handle filled with the palette `Button` color and outlined with the palette `Dark` color).
- B13: given a `QCheckBox`, a `QRadioButton`, a `QLineEdit` and a `QProgressBar`, rendered with the style, the result is no image (these parts keep Fusion's vector drawing or get a flat one).
- B14: given a disabled `QPushButton` rendered with the style, the result is that the fill and outline colors are the ones of the palette `Disabled` group.

### Applying to a window (`hpq::apply_theme`, `hpq::remove_theme`)
- B15: given a `QMainWindow` with child widgets and `apply_theme(window, theme, geometry_engine)`, the result is that the window and all its existing descendant widgets use one `HandPaintStyle` (the same object for all). This replaces the "Fusion style" point of behavior B21 of `specs/sketch-paint-engine`; the other points of B21 stay.
- B16: given `remove_theme(window)` after `apply_theme`, the result is that the window and its descendants are back to the style they had before `apply_theme` (B25 of `specs/sketch-paint-engine` still holds).
- B17: given the example `MainWindow` themed and grabbed with `QT_QPA_PLATFORM=offscreen`, the result is that it paints with no crash and no Qt warning, and the push button, combobox and group box frame are drawn with rough outlines (seen in the grab by the real tester).

## Out of scope
- Rough or hand-drawn shapes inside the style: the style draws exact shapes, the `HandPaintEngine` makes them rough.
- Gradients, shadows, highlights and rounded corners of Fusion: parts are flat.
- Standard icons (`standardIcon`, `standardPixmap`: message box icons, title bar buttons, file icons) and widget icons: they stay images.
- Parts that Fusion already draws with vector primitives and that are not listed above keep Fusion's drawing.
- Styles other than Fusion as the base style, and different looks per theme (China Ink, Black slate and chalk).
- Widgets created after `apply_theme` (they still do not get the style).

## Test command
`cmake --preset debug && cmake --build --preset debug && QT_QPA_PLATFORM=offscreen ctest --preset debug`
