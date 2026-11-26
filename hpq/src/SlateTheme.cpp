#include <hpq/Themes.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

PaletteColors slate_colors(bool is_disabled) {
    const QColor slate(0x24, 0x29, 0x2b);
    const QColor chalk = is_disabled ? QColor(0x7c, 0x82, 0x80) : QColor(0xf3, 0xef, 0xe4);
    const QColor chalk_dim = is_disabled ? QColor(0x66, 0x6c, 0x6b) : QColor(0xb4, 0xb5, 0xae);
    const QColor chalk_smear(0x4a, 0x51, 0x54);

    PaletteColors colors(slate, chalk);
    colors.alternate_base = colors.midlight = chalk_smear;
    colors.mid = colors.dark = chalk_dim;
    colors.highlight = is_disabled ? QColor(0x55, 0x60, 0x66) : QColor(0x5b, 0x9b, 0xd5);
    colors.highlighted_text = QColor(0xff, 0xff, 0xff);
    colors.link = is_disabled ? QColor(0x62, 0x7a, 0x8c) : QColor(0x9c, 0xd0, 0xff);
    colors.placeholder_text = QColor(0x8c, 0x91, 0x8e);
    return colors;
}

// A piece of chalk draws one thick line: a second stroke would only blur it.
constexpr RoughOptions ChalkRoughOptions{1.0, 1, 1.5};

}  // namespace

Theme slate_theme(FillStyle fill_style) {
    Theme theme;
    theme.palette = make_palette(slate_colors(false), slate_colors(true));
    theme.font = QFont(hand_font_family());
    theme.rough_options = ChalkRoughOptions;
    theme.fill_style = fill_style;
    theme.medium = Medium::Chalk;
    return theme;
}

}  // namespace hpq
