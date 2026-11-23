#include <hpq/WatercolorTheme.h>

#include "HandFont.h"

#include <QColor>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor pale_wash;
    QColor paint;
    QColor line;
    QColor pale_line;
    QColor selection_wash;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group) {
    GroupColors colors{
        QColor(0xf8, 0xf5, 0xee),  // paper
        QColor(0xf1, 0xe1, 0xbf),  // pale_wash: diluted raw sienna
        QColor(0x2c, 0x36, 0x45),  // paint: Payne's grey, hardly diluted
        QColor(0x4a, 0x5a, 0x6e),  // line: Payne's grey
        QColor(0x97, 0xa3, 0xb0),  // pale_line
        QColor(0x9c, 0xcb, 0xe4),  // selection_wash: cerulean blue
        QColor(0x2f, 0x5f, 0xa8),  // link: ultramarine
        QColor(0xa3, 0xa8, 0xad),  // placeholder
    };
    // A disabled widget is painted with what is left in the brush after it was rinsed.
    if (group == QPalette::Disabled) {
        colors.paint = QColor(0xa4, 0xab, 0xb3);
        colors.line = QColor(0xb0, 0xb7, 0xbf);
        colors.selection_wash = QColor(0xd3, 0xe1, 0xe8);
        colors.link = QColor(0xa9, 0xb9, 0xd3);
    }
    return colors;
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
        return colors.pale_wash;
    case QPalette::Mid:
        return colors.pale_line;
    case QPalette::Dark:
        return colors.line;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.selection_wash;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // WindowText, Text, ButtonText, HighlightedText, ToolTipText, Shadow and any later text role.
        return colors.paint;
    }
}

QPalette watercolor_palette() {
    QPalette palette;
    for (const QPalette::ColorGroup group : PaletteGroups) {
        const GroupColors colors = colors_for(group);
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            palette.setColor(group, color_role, color_for(color_role, colors));
        }
    }
    return palette;
}

// A paint brush draws a line in one stroke, and a wash overflows its outline a little. Its
// hachures are wider apart than those of a pencil.
constexpr RoughOptions PaintBrushRoughOptions{1.0, 1, 2.0, 5.5};

}  // namespace

Theme watercolor_theme(FillStyle fill_style) {
    return Theme(watercolor_palette(), QFont(hand_font_family()), PaintBrushRoughOptions, fill_style,
                 Medium::Watercolor);
}

}  // namespace hpq
