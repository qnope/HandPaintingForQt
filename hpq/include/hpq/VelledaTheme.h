#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// Dry-erase markers on a white board: black for the text, blue for the frames, red for the
// selection and green for the links.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// With FillStyle::Hachure, the text of a selection is black and not white: it is read on the
// board that shows between the hachures.
HPQ_EXPORT Theme velleda_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
