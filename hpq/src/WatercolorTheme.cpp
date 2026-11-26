#include <hpq/Themes.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

// A disabled widget is painted with what is left in the brush after it was rinsed.
PaletteColors watercolor_colors(bool is_disabled) {
    const QColor paper(0xf8, 0xf5, 0xee);
    // Payne's grey, hardly diluted.
    const QColor paint = is_disabled ? QColor(0xa4, 0xab, 0xb3) : QColor(0x2c, 0x36, 0x45);
    // Payne's grey.
    const QColor line = is_disabled ? QColor(0xb0, 0xb7, 0xbf) : QColor(0x4a, 0x5a, 0x6e);
    const QColor pale_line(0x97, 0xa3, 0xb0);
    // Diluted raw sienna.
    const QColor pale_wash(0xf1, 0xe1, 0xbf);
    // Cerulean blue.
    const QColor selection_wash = is_disabled ? QColor(0xd3, 0xe1, 0xe8) : QColor(0x9c, 0xcb, 0xe4);
    const QColor ultramarine = is_disabled ? QColor(0xa9, 0xb9, 0xd3) : QColor(0x2f, 0x5f, 0xa8);

    PaletteColors colors(paper, paint);
    colors.alternate_base = colors.midlight = pale_wash;
    colors.mid = pale_line;
    colors.dark = line;
    colors.highlight = selection_wash;
    colors.link = ultramarine;
    colors.placeholder_text = QColor(0xa3, 0xa8, 0xad);
    return colors;
}

// A paint brush draws a line in one stroke, and a wash overflows its outline a little. Its
// hachures are wider apart than those of a pencil.
constexpr RoughOptions PaintBrushRoughOptions{1.0, 1, 2.0, 5.5};

}  // namespace

Theme watercolor_theme(FillStyle fill_style) {
    Theme theme;
    theme.palette = make_palette(watercolor_colors(false), watercolor_colors(true));
    theme.font = QFont(hand_font_family());
    theme.rough_options = PaintBrushRoughOptions;
    theme.fill_style = fill_style;
    theme.medium = Medium::Watercolor;
    return theme;
}

}  // namespace hpq
