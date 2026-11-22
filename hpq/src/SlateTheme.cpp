#include <hpq/SlateTheme.h>

#include "HandFont.h"

#include <QColor>

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

QColor color_for(QPalette::ColorRole role, const GroupColors &colors) {
    switch (role) {
    case QPalette::Window:
    case QPalette::Base:
    case QPalette::Button:
    case QPalette::Light:
    case QPalette::BrightText:
    case QPalette::ToolTipBase:
        return colors.slate;
    case QPalette::AlternateBase:
    case QPalette::Midlight:
        return colors.chalk_smear;
    case QPalette::Mid:
    case QPalette::Dark:
        return colors.chalk_dim;
    case QPalette::Shadow:
        return colors.chalk;
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
        return colors.chalk;
    }
}

QPalette slate_palette() {
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

// A piece of chalk draws one thick line: a second stroke would only blur it.
constexpr RoughOptions ChalkRoughOptions{1.0, 1, 1.5};

}  // namespace

Theme slate_theme(FillStyle fill_style) {
    return Theme(slate_palette(), QFont(hand_font_family()), ChalkRoughOptions, fill_style, Medium::Chalk);
}

}  // namespace hpq
