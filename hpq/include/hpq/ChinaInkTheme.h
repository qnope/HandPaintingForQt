#pragma once

#include <hpq/Theme.h>

namespace hpq {

// China ink laid with a brush on rice paper: black lines that several passes of the brush
// make thicker and thinner, washes of diluted ink, and the red of a seal for the selection.
// Registers the embedded "Kaushan Script" font in QFontDatabase.
// With FillStyle::Hachure, the text of a selection is ink and not paper: it is read on the
// paper that shows between the hachures.
Theme china_ink_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
