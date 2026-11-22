#pragma once

#include "RecordingPaintDevice.h"

#include <vector>

class QWidget;

namespace hpq::test {

// Renders the widget with the SketchTheme palette and a HandPaintStyle (no HandPaintEffect)
// with QWidget::render on a RecordingPaintDevice, and returns the recorded draw calls.
// The widget and its descendants get back the application style afterwards.
std::vector<DrawCall> render_with_style(QWidget &widget);

// True when the calls draw an image: drawPixmap, drawImage or drawTiledPixmap.
bool has_image(const std::vector<DrawCall> &calls);

}  // namespace hpq::test
