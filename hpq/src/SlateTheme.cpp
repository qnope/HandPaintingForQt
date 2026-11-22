#include <hpq/SlateTheme.h>

#include "ChalkTexture.h"
#include "HandFont.h"

#include <QBrush>
#include <QColor>
#include <QGuiApplication>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor slate;
    QColor chalk_smear;
    QColor chalk;
    QColor chalk_dim;
    QColor highlight;
    QColor highlighted_text;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group) {
    GroupColors colors{
        QColor(0x24, 0x29, 0x2b),  // slate
        QColor(0x4a, 0x51, 0x54),  // chalk_smear
        QColor(0xf3, 0xef, 0xe4),  // chalk
        QColor(0xb4, 0xb5, 0xae),  // chalk_dim
        QColor(0x5b, 0x9b, 0xd5),  // highlight
        QColor(0xff, 0xff, 0xff),  // highlighted_text
        QColor(0x9c, 0xd0, 0xff),  // link
        QColor(0x8c, 0x91, 0x8e),  // placeholder
    };
    if (group == QPalette::Disabled) {
        colors.chalk = QColor(0x7c, 0x82, 0x80);
        colors.chalk_dim = QColor(0x66, 0x6c, 0x6b);
        colors.highlight = QColor(0x55, 0x60, 0x66);
        colors.link = QColor(0x62, 0x7a, 0x8c);
    }
    return colors;
}

QBrush brush_for(QPalette::ColorRole role, const GroupColors &colors, double device_pixel_ratio) {
    const auto chalk = [&](const QColor &color, ChalkGrain grain) {
        return chalk_brush(color, grain, device_pixel_ratio);
    };
    switch (role) {
    case QPalette::Window:
    case QPalette::Base:
    case QPalette::Button:
    case QPalette::Light:
    case QPalette::BrightText:
    case QPalette::ToolTipBase:
        return slate_brush(colors.slate, device_pixel_ratio);
    case QPalette::AlternateBase:
    case QPalette::Midlight:
        return chalk(colors.chalk_smear, ChalkGrain::Fill);
    case QPalette::Mid:
    case QPalette::Dark:
        return chalk(colors.chalk_dim, ChalkGrain::Stroke);
    case QPalette::Shadow:
        return chalk(colors.chalk, ChalkGrain::Stroke);
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return chalk(colors.highlight, ChalkGrain::Fill);
    case QPalette::HighlightedText:
        return chalk(colors.highlighted_text, ChalkGrain::Text);
    case QPalette::Link:
    case QPalette::LinkVisited:
        return chalk(colors.link, ChalkGrain::Text);
    case QPalette::PlaceholderText:
        return chalk(colors.placeholder, ChalkGrain::Text);
    default:  // WindowText, Text, ButtonText, ToolTipText and any later text role.
        return chalk(colors.chalk, ChalkGrain::Text);
    }
}

// The device pixel ratio the textures are made for: the highest one of the screens.
double device_pixel_ratio() {
    return qGuiApp ? qGuiApp->devicePixelRatio() : 1.0;
}

// Every role is a textured brush whose color is the color of the chalk or of the slate: what
// is drawn with the brush gets the grain, what only reads its color is plain.
QPalette slate_palette() {
    const double pixel_ratio = device_pixel_ratio();
    QPalette palette;
    for (const QPalette::ColorGroup group : PaletteGroups) {
        const GroupColors colors = colors_for(group);
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            palette.setBrush(group, color_role, brush_for(color_role, colors, pixel_ratio));
        }
    }
    return palette;
}

// Two strokes close to each other: the thin lines of the widgets read as one thicker line of
// chalk, uneven where the strokes part.
constexpr RoughOptions ChalkRoughOptions{1.0, 2, 1.2};

}  // namespace

Theme slate_theme(FillStyle fill_style) {
    return Theme(slate_palette(), QFont(hand_font_family()), ChalkRoughOptions, fill_style);
}

}  // namespace hpq
