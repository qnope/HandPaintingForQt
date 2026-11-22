#pragma once

#include <hpq/FwdDecls.h>

#include <memory>

class QWidget;

namespace hpq {

// Gives `window` the look of `theme`: its palette and font on the window, one HandPaintStyle
// on the window and its existing descendant widgets, and a HandPaintEffect on the window.
// The lists that its combo boxes open are windows of their own: each gets the palette and a
// HandPaintEffect too.
void apply_theme(QWidget &window, const Theme &theme,
                 std::shared_ptr<const GeometryEngine> geometry_engine);

// Removes the HandPaintEffect of `window` and of the lists of its combo boxes, and gives back
// the palettes, font and styles they had before the first apply_theme. Does nothing on a window that was never themed.
void remove_theme(QWidget &window);

}  // namespace hpq
