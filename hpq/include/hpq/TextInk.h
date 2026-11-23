#pragma once

namespace hpq {

// What a theme writes its text with.
enum class TextInk {
    // An ink that shows: every text is drawn, in the color of its pen.
    Visible,
    // An ink that shows only where it is selected: a text is drawn only when the color of its
    // pen is the HighlightedText color of the palette of the theme, in any of its color groups.
    Invisible,
};

}  // namespace hpq
