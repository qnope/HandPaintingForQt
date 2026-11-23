#pragma once

#include <hpq/Theme.h>

namespace hpq {

// Notes written with a black pen on white paper, and marked with fluorescent highlighters:
// yellow for the selection, green for the buttons and the headers, pink for the frames and a
// light pass of blue for the alternate rows.
// Registers the embedded "Architects Daughter" font in QFontDatabase.
// A highlighter lets what is under it show: its colors are translucent, and the text of a
// selection stays the one of the pen.
// With FillStyle::Hachure the colors are opaque, so that the fills are hatched, and the
// selection is orange: the thin hachures of the yellow highlighter would not show on the paper.
Theme highlighter_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
