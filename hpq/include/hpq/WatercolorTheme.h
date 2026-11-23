#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// Watercolor on a grainy white paper: lines drawn with the tip of a paint brush in Payne's
// grey, and washes of diluted color with a soft edge where the pigment gathers. A wash lets
// what is under it show through: the text of a selection is dark, on a wash of blue.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
HPQ_EXPORT Theme watercolor_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
