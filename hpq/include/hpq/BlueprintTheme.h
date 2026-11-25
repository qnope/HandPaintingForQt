#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// White ink on the blue paper of an architect's blueprint, printed with a faint grid.
// Registers the embedded "Architects Daughter" font in QFontDatabase, and letters in capitals:
// make the theme once the QApplication exists.
// The grid is a texture of the Window, Base and Button brushes: their color is the blue of the paper.
// With FillStyle::Hachure, the text of a selection is ink and not paper: it is read on the
// paper that shows between the hachures.
HPQ_EXPORT Theme blueprint_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
