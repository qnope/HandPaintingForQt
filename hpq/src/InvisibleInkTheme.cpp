#include <hpq/InvisibleInkTheme.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

// The lamp is turned low on a disabled widget.
PaletteColors invisible_ink_colors(bool is_disabled, FillStyle fill_style) {
    const QColor paper(0xf8, 0xf5, 0xec);
    // The arrows, the check marks and the cursor of a text.
    const QColor pencil = is_disabled ? QColor(0xb4, 0xb0, 0xa6) : QColor(0x55, 0x52, 0x4c);
    // The frames.
    const QColor pencil_light(0x9a, 0x96, 0x8c);

    PaletteColors colors(paper, pencil);
    colors.alternate_base = colors.midlight = QColor(0xf0, 0xec, 0xe0);
    colors.mid = colors.dark = pencil_light;
    // The violet light of the lamp on a selection, where the ink glows.
    colors.highlight = is_disabled ? QColor(0x9d, 0x8e, 0xc9) : QColor(0x44, 0x1f, 0xa3);
    colors.highlighted_text = is_disabled ? QColor(0xe2, 0xf6, 0xf8) : QColor(0x9f, 0xf5, 0xff);
    colors.link = is_disabled ? QColor(0xa4, 0xac, 0xc4) : QColor(0x4a, 0x5f, 0x9a);
    colors.placeholder_text = QColor(0x9a, 0x96, 0x8c);
    // The paper shows between the hachures of a selection: its text is read on the paper, in
    // the violet of the lamp, through hachures light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = is_disabled ? QColor(0x9d, 0x8e, 0xc9) : QColor(0x3b, 0x16, 0x96);
        colors.highlight = is_disabled ? QColor(0xd6, 0xcd, 0xee) : QColor(0xab, 0x92, 0xe8);
    }
    return colors;
}

}  // namespace

Theme invisible_ink_theme(FillStyle fill_style) {
    return Theme(make_palette(invisible_ink_colors(false, fill_style), invisible_ink_colors(true, fill_style)),
                 QFont(hand_font_family()), RoughOptions{}, fill_style, Medium::Pencil, TextInk::Invisible);
}

}  // namespace hpq
