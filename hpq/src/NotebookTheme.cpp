#include <hpq/Themes.h>

#include "Fonts.h"
#include "SeyesPaper.h"
#include "ThemePalette.h"

#include <QBrush>
#include <QColor>

namespace hpq {

namespace {

constexpr QColor Paper(0xfd, 0xfd, 0xf9);
constexpr QColor Pencil(0x9c, 0x9c, 0xa2);
constexpr QColor PencilLight(0xbc, 0xbc, 0xc2);

// The ruled sheet as a brush of the color of the paper. Every theme shares one image: two
// brushes are equal only when they share it, and so are the palettes of two themes.
QBrush sheet_brush() {
    static const QImage sheet = seyes_sheet({
        Paper,
        QColor(0xb4, 0xbe, 0xea),  // line
        QColor(0xdd, 0xe2, 0xf6),  // fine_line
        QColor(0xf0, 0x86, 0x94),  // margin
    });
    QBrush brush(sheet);
    // The image has its own colors: the color of the brush tells what the styles take for it.
    brush.setColor(Paper);
    return brush;
}

// What is disabled is written with a pencil.
PaletteColors notebook_colors(bool is_disabled, FillStyle fill_style) {
    const QColor ink = is_disabled ? Pencil : QColor(0x1d, 0x35, 0x9a);
    const QColor ink_light = is_disabled ? PencilLight : QColor(0x4f, 0x69, 0xc6);
    const QColor ink_pale = is_disabled ? PencilLight : QColor(0x93, 0xa4, 0xde);
    // The ruling of the sheet shows through a field and a button.
    const QColor see_through(0xfd, 0xfd, 0xf9, 0);
    const QColor wash = is_disabled ? QColor(0x80, 0x80, 0x88, 0x1c) : QColor(0x2b, 0x4a, 0xc0, 0x26);
    const QColor highlighter = is_disabled ? QColor(0xd9, 0xd9, 0xd4, 0xb4) : QColor(0xff, 0xe0, 0x2a, 0xc0);
    const QColor colored_pencil = is_disabled ? PencilLight : QColor(0xf2, 0xa9, 0x00);
    const QColor sticky_note(0xff, 0xf4, 0x9c);

    PaletteColors colors(Paper, ink);
    colors.window = sheet_brush();
    colors.base = colors.button = see_through;
    colors.tool_tip_base = sticky_note;
    colors.alternate_base = colors.midlight = wash;
    colors.mid = ink_pale;
    colors.dark = ink_light;
    // A hatched fill is opaque: the highlighter gives way to a colored pencil, whose thin lines
    // show on the paper.
    colors.highlight = fill_style == FillStyle::Hachure ? colored_pencil : highlighter;
    colors.link = is_disabled ? QColor(0xd8, 0xa9, 0xa9) : QColor(0xcf, 0x2a, 0x2a);
    colors.placeholder_text = Pencil;
    return colors;
}

// A pen draws one line, and a pupil draws it with care.
constexpr RoughOptions PenRoughOptions{0.8, 1, 1.5};

// The school handwriting has tall capitals and long ascenders: at this fraction of the size of
// the application font, its small letters are as high as those of the application font would be.
constexpr double HandwritingScale = 0.85;

// The school font, at the size of the application font scaled by HandwritingScale.
QFont handwriting_font() {
    const QFont application_font;
    QFont font(school_font_family());
    if (application_font.pointSizeF() > 0) {
        font.setPointSizeF(application_font.pointSizeF() * HandwritingScale);
    } else if (application_font.pixelSize() > 0) {
        font.setPixelSize(qRound(application_font.pixelSize() * HandwritingScale));
    }
    return font;
}

}  // namespace

Theme notebook_theme(FillStyle fill_style) {
    Theme theme;
    theme.palette = make_palette(notebook_colors(false, fill_style), notebook_colors(true, fill_style));
    theme.font = handwriting_font();
    theme.rough_options = PenRoughOptions;
    theme.fill_style = fill_style;
    return theme;
}

}  // namespace hpq
