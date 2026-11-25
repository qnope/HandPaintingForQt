#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// White and colored chalk on a black slate.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// With FillStyle::Hachure, fills are hatched with chalk on the slate.
HPQ_EXPORT Theme slate_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
