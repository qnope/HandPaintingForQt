#pragma once

#include <hpq/Theme.h>

namespace hpq {

// White chalk on a black slate. The roles of its palette are textured brushes: text, outlines
// and fills drawn with them get the grain of the chalk, and backgrounds the dust of the slate.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// With FillStyle::Hachure, the fills that are not from the palette are hatched on the slate.
Theme slate_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
