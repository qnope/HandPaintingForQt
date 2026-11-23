#pragma once

#include <hpq/Theme.h>

namespace hpq {

// A letter written in invisible ink: a pencil draws the frames and the marks, and the text
// shows only where it is selected, under the violet light of a lamp.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// The text is written with TextInk::Invisible: HighlightedText is the only color in which a
// text is drawn, and no other text role of the palette has it.
// With FillStyle::Hachure, the text of a selection is violet and not luminous: it is read on
// the paper that shows between the hachures.
Theme invisible_ink_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
