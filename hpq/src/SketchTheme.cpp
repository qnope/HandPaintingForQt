#include <hpq/SketchTheme.h>

#include "HandFont.h"

#include <QColor>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor paper_alternate;
    QColor graphite;
    QColor graphite_light;
    QColor highlight;
    QColor highlighted_text;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(0xfb, 0xf8, 0xef),  // paper
        QColor(0xf2, 0xed, 0xdf),  // paper_alternate
        QColor(0x2b, 0x2b, 0x2b),  // graphite
        QColor(0x8a, 0x86, 0x7d),  // graphite_light
        QColor(0x4a, 0x4a, 0x4a),  // highlight
        QColor(0xfb, 0xf8, 0xef),  // highlighted_text
        QColor(0x2a, 0x4a, 0x7a),  // link
        QColor(0x9a, 0x96, 0x8c),  // placeholder
    };
    if (group == QPalette::Disabled) {
        colors.graphite = QColor(0xa8, 0xa4, 0x9a);
        colors.highlight = QColor(0xb8, 0xb4, 0xaa);
        colors.link = QColor(0x9a, 0xa4, 0xb4);
    }
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = colors.graphite;
        colors.highlight = group == QPalette::Disabled ? QColor(0xcf, 0xcb, 0xc1) : colors.graphite_light;
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
        return colors.paper_alternate;
    case QPalette::Mid:
    case QPalette::Dark:
        return colors.graphite_light;
    case QPalette::Shadow:
        return colors.graphite;
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
        return colors.graphite;
    }
}

QPalette sketch_palette(FillStyle fill_style) {
    QPalette palette;
    for (const QPalette::ColorGroup group : PaletteGroups) {
        const GroupColors colors = colors_for(group, fill_style);
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            palette.setColor(group, color_role, color_for(color_role, colors));
        }
    }
    return palette;
}

}  // namespace

Theme sketch_theme(FillStyle fill_style) {
    return Theme(sketch_palette(fill_style), QFont(hand_font_family()), RoughOptions{}, fill_style);
}

}  // namespace hpq
