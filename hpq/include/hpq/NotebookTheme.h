#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// A fountain pen on the Seyes ruled paper of a French school notebook.
// Registers the embedded "Playwrite FR Moderne" font in QFontDatabase.
// The window brush is the ruled sheet; views and buttons are see-through, so that the ruling
// runs under them. A selection is a stroke of highlighter: with FillStyle::Hachure, it is
// hatched with a colored pencil instead.
HPQ_EXPORT Theme notebook_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
