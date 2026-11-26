#include <hpq/ChinaInkTheme.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

PaletteColors china_ink_colors(bool is_disabled, FillStyle fill_style) {
    const QColor paper(0xf3, 0xec, 0xda);
    const QColor ink = is_disabled ? QColor(0xa9, 0xa4, 0x98) : QColor(0x1b, 0x1a, 0x1c);
    const QColor ink_line = is_disabled ? QColor(0xa9, 0xa4, 0x98) : QColor(0x2e, 0x2c, 0x2a);
    const QColor ink_pale(0x8d, 0x88, 0x7c);
    const QColor wash(0xdc, 0xd7, 0xc9);
    // The red of a seal.
    const QColor seal = is_disabled ? QColor(0xcd, 0xb9, 0xb0) : QColor(0xb8, 0x32, 0x2a);

    PaletteColors colors(paper, ink);
    colors.alternate_base = colors.midlight = wash;
    colors.mid = ink_pale;
    colors.dark = ink_line;
    colors.highlight = seal;
    colors.highlighted_text = paper;
    colors.link = is_disabled ? QColor(0x9a, 0xa4, 0xb4) : QColor(0x2c, 0x4a, 0x6e);
    colors.placeholder_text = QColor(0x9c, 0x97, 0x8a);
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures of a red diluted enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = ink;
        colors.highlight = is_disabled ? QColor(0xdc, 0xc8, 0xc0) : QColor(0xcc, 0x55, 0x48);
    }
    return colors;
}

// A brush draws a line in one stroke. Its hachures are wider apart than those of a pencil.
constexpr RoughOptions BrushRoughOptions{1.0, 1, 1.5, 5.0};

}  // namespace

Theme china_ink_theme(FillStyle fill_style) {
    return Theme(make_palette(china_ink_colors(false, fill_style), china_ink_colors(true, fill_style)),
                 QFont(brush_font_family()), BrushRoughOptions, fill_style, Medium::Ink);
}

}  // namespace hpq
