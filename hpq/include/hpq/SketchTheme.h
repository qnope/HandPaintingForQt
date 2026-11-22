#pragma once

#include <hpq/Theme.h>

namespace hpq {

// Graphite pencil on paper.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// With FillStyle::Hachure, the text of a selection is graphite and not paper: it is read
// on the paper that shows between the hachures.
Theme sketch_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
