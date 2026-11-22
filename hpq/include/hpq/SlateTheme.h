#pragma once

#include <hpq/Theme.h>

namespace hpq {

// White and colored chalk on a black slate.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// With FillStyle::Hachure, fills are hatched with chalk on the slate.
Theme slate_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
