#include <hpq/BlueprintTheme.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QBrush>
#include <QColor>
#include <QImage>

namespace hpq {

namespace {

// The blue of the paper, and the lines printed on it.
constexpr QRgb PaperBlue = 0xff17468c;
constexpr QRgb GridLine = 0xff245494;
constexpr QRgb GridHeavyLine = 0xff32629f;

// Side, in pixels, of a square of the grid.
constexpr int GridStep = 10;
// One line out of this many is heavier.
constexpr int GridHeavyEvery = 5;

// One tile of the grid: the lines along its top and left edges are heavy.
QImage grid_tile() {
    const int side = GridStep * GridHeavyEvery;
    QImage tile(side, side, QImage::Format_RGB32);
    for (int y = 0; y < side; ++y) {
        for (int x = 0; x < side; ++x) {
            const bool on_heavy_line = x == 0 || y == 0;
            const bool on_line = x % GridStep == 0 || y % GridStep == 0;
            tile.setPixel(x, y, on_heavy_line ? GridHeavyLine : on_line ? GridLine : PaperBlue);
        }
    }
    return tile;
}

// The paper with its grid. Every call shares one image: two brushes with a texture are equal
// only when they share it, and so are two palettes of the theme.
QBrush paper_brush(const QColor &paper) {
    static const QImage tile = grid_tile();
    QBrush brush(paper);
    brush.setTextureImage(tile);
    return brush;
}

PaletteColors blueprint_colors(bool is_disabled, FillStyle fill_style) {
    const QColor paper(PaperBlue);
    const QColor ink = is_disabled ? QColor(0x7c, 0x9c, 0xcc) : QColor(0xf2, 0xf8, 0xff);
    const QColor ink_dim = is_disabled ? QColor(0x5c, 0x80, 0xb8) : QColor(0xa4, 0xc8, 0xf0);

    PaletteColors colors(paper, ink);
    // The grid shows where the widgets are drawn, not on a tool tip.
    colors.window = colors.base = colors.button = paper_brush(paper);
    colors.alternate_base = colors.midlight = QColor(0x2a, 0x5c, 0xa8);
    colors.mid = colors.dark = ink_dim;
    colors.highlight = is_disabled ? QColor(0x6c, 0x8e, 0xc2) : QColor(0xd4, 0xe8, 0xff);
    colors.highlighted_text = paper;
    colors.link = is_disabled ? QColor(0x6c, 0x9c, 0xc4) : QColor(0x8f, 0xe4, 0xff);
    colors.placeholder_text = QColor(0x86, 0xa8, 0xd6);
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures dim enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = ink;
        colors.highlight = is_disabled ? QColor(0x3c, 0x64, 0xa4) : QColor(0x5a, 0x8a, 0xd0);
    }
    return colors;
}

// A draughtsman draws along a ruler: one stroke that hardly leaves the line, and hachures at 45 degrees.
constexpr RoughOptions DraughtsmanRoughOptions{0.6, 1, 1.5, 5.0, -45.0};

// The lettering of a plan is in capitals.
QFont lettering_font() {
    QFont font(hand_font_family());
    font.setCapitalization(QFont::AllUppercase);
    return font;
}

}  // namespace

Theme blueprint_theme(FillStyle fill_style) {
    return Theme(make_palette(blueprint_colors(false, fill_style), blueprint_colors(true, fill_style)), lettering_font(),
                 DraughtsmanRoughOptions, fill_style);
}

}  // namespace hpq
