#include <hpq/BlueprintTheme.h>

#include "HandFont.h"

#include <QBrush>
#include <QColor>
#include <QImage>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor paper_alternate;
    QColor ink;
    QColor ink_dim;
    QColor highlight;
    QColor highlighted_text;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

// The blue of the paper, and the lines printed on it.
constexpr QRgb PaperBlue = 0xff17468c;
constexpr QRgb GridLine = 0xff245494;
constexpr QRgb GridHeavyLine = 0xff32629f;

// Side, in pixels, of a square of the grid.
constexpr int GridStep = 10;
// One line out of this many is heavier.
constexpr int GridHeavyEvery = 5;

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(PaperBlue),         // paper
        QColor(0x2a, 0x5c, 0xa8),  // paper_alternate
        QColor(0xf2, 0xf8, 0xff),  // ink
        QColor(0xa4, 0xc8, 0xf0),  // ink_dim
        QColor(0xd4, 0xe8, 0xff),  // highlight
        QColor(PaperBlue),         // highlighted_text
        QColor(0x8f, 0xe4, 0xff),  // link
        QColor(0x86, 0xa8, 0xd6),  // placeholder
    };
    if (group == QPalette::Disabled) {
        colors.ink = QColor(0x7c, 0x9c, 0xcc);
        colors.ink_dim = QColor(0x5c, 0x80, 0xb8);
        colors.highlight = QColor(0x6c, 0x8e, 0xc2);
        colors.link = QColor(0x6c, 0x9c, 0xc4);
    }
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures dim enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = colors.ink;
        colors.highlight = group == QPalette::Disabled ? QColor(0x3c, 0x64, 0xa4) : QColor(0x5a, 0x8a, 0xd0);
    }
    return colors;
}

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

bool is_paper(QPalette::ColorRole role) {
    return role == QPalette::Window || role == QPalette::Base || role == QPalette::Button;
}

QColor color_for(QPalette::ColorRole role, const GroupColors &colors) {
    switch (role) {
    case QPalette::Window:
    case QPalette::Base:
    case QPalette::Button:
    case QPalette::Light:
    case QPalette::BrightText:
    case QPalette::ToolTipBase:
        return colors.paper;
    case QPalette::AlternateBase:
    case QPalette::Midlight:
        return colors.paper_alternate;
    case QPalette::Mid:
    case QPalette::Dark:
        return colors.ink_dim;
    case QPalette::Shadow:
        return colors.ink;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.highlight;
    case QPalette::HighlightedText:
        return colors.highlighted_text;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // WindowText, Text, ButtonText, ToolTipText and any later text role.
        return colors.ink;
    }
}

QPalette blueprint_palette(FillStyle fill_style) {
    QPalette palette;
    for (const QPalette::ColorGroup group : PaletteGroups) {
        const GroupColors colors = colors_for(group, fill_style);
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            const QColor color = color_for(color_role, colors);
            palette.setBrush(group, color_role, is_paper(color_role) ? paper_brush(color) : QBrush(color));
        }
    }
    return palette;
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
    return Theme(blueprint_palette(fill_style), lettering_font(), DraughtsmanRoughOptions, fill_style);
}

}  // namespace hpq
