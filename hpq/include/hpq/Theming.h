#pragma once

#include <hpq/FwdDecls.h>

#include <memory>

class QWidget;

namespace hpq {

// Gives `window` the look of `theme`: its palette and font on the window, the Fusion style
// on the window and its existing descendant widgets, and a HandPaintEffect on the window.
void apply_theme(QWidget &window, std::shared_ptr<const Theme> theme,
                 std::shared_ptr<const GeometryEngine> geometry_engine);

// Removes the HandPaintEffect of `window` and gives back the palette, font and styles it had
// before the first apply_theme. Does nothing on a window that was never themed.
void remove_theme(QWidget &window);

}  // namespace hpq
