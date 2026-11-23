#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>

#include <memory>

class QWidget;

namespace hpq {

// Gives `widget` the look of `theme`: its palette and font on the widget, one HandPaintStyle
// on the widget and its existing descendant widgets, and a HandPaintEffect on the widget.
// Its existing labels get the marker of the theme, if it has one.
// The windows that the widget opens get the same look, each with a HandPaintEffect of its own:
// its menus, its dialogs, its tooltips and the lists of its combo boxes, already open or opened
// afterwards. A window is opened by its parent widget, and a tooltip without parent (as under
// Windows) by the widget that asked for it. A window opened inside a descendant given another
// theme takes the theme of that descendant.
HPQ_EXPORT void apply_theme(QWidget &widget, const Theme &theme,
                            std::shared_ptr<const GeometryEngine> geometry_engine);

// Removes the HandPaintEffect of `widget` and of the windows it opened, takes the markers away
// from their labels, and gives back the palettes, fonts and styles they had before the first
// apply_theme. The windows it opens afterwards are no longer themed. Does nothing on a widget
// that was never themed.
HPQ_EXPORT void remove_theme(QWidget &widget);

}  // namespace hpq
