#include <hpq/Themes.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

PaletteColors sketch_colors(bool is_disabled, FillStyle fill_style) {
    const QColor paper(0xfb, 0xf8, 0xef);
    const QColor graphite = is_disabled ? QColor(0xa8, 0xa4, 0x9a) : QColor(0x2b, 0x2b, 0x2b);
    const QColor graphite_light(0x8a, 0x86, 0x7d);

    PaletteColors colors(paper, graphite);
    colors.alternate_base = colors.midlight = QColor(0xf2, 0xed, 0xdf);
    colors.mid = colors.dark = graphite_light;
    colors.highlight = is_disabled ? QColor(0xb8, 0xb4, 0xaa) : QColor(0x4a, 0x4a, 0x4a);
    colors.highlighted_text = paper;
    colors.link = is_disabled ? QColor(0x9a, 0xa4, 0xb4) : QColor(0x2a, 0x4a, 0x7a);
    colors.placeholder_text = QColor(0x9a, 0x96, 0x8c);
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = graphite;
        colors.highlight = is_disabled ? QColor(0xcf, 0xcb, 0xc1) : graphite_light;
    }
    return colors;
}

}  // namespace

Theme sketch_theme(FillStyle fill_style) {
    Theme theme;
    theme.palette = make_palette(sketch_colors(false, fill_style), sketch_colors(true, fill_style));
    theme.font = QFont(hand_font_family());
    theme.fill_style = fill_style;
    return theme;
}

}  // namespace hpq
