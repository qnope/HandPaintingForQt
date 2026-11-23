#include <hpq/InvisibleInkTheme.h>

#include "HandFont.h"

#include <QColor>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor paper_alternate;
    QColor pencil;
    QColor pencil_light;
    QColor lamp;
    QColor revealed_ink;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(0xf8, 0xf5, 0xec),  // paper
        QColor(0xf0, 0xec, 0xe0),  // paper_alternate
        QColor(0x55, 0x52, 0x4c),  // pencil: the arrows, the check marks and the cursor of a text
        QColor(0x9a, 0x96, 0x8c),  // pencil_light: the frames
        QColor(0x44, 0x1f, 0xa3),  // lamp: the violet light on a selection
        QColor(0x9f, 0xf5, 0xff),  // revealed_ink: the ink glows under the lamp
        QColor(0x4a, 0x5f, 0x9a),  // link
        QColor(0x9a, 0x96, 0x8c),  // placeholder
    };
    // The lamp is turned low on a disabled widget.
    if (group == QPalette::Disabled) {
        colors.pencil = QColor(0xb4, 0xb0, 0xa6);
        colors.lamp = QColor(0x9d, 0x8e, 0xc9);
        colors.revealed_ink = QColor(0xe2, 0xf6, 0xf8);
        colors.link = QColor(0xa4, 0xac, 0xc4);
    }
    // The paper shows between the hachures of a selection: its text is read on the paper, in
    // the violet of the lamp, through hachures light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        const bool is_disabled = group == QPalette::Disabled;
        colors.revealed_ink = is_disabled ? QColor(0x9d, 0x8e, 0xc9) : QColor(0x3b, 0x16, 0x96);
        colors.lamp = is_disabled ? QColor(0xd6, 0xcd, 0xee) : QColor(0xab, 0x92, 0xe8);
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
        return colors.pencil_light;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.lamp;
    case QPalette::HighlightedText:
        return colors.revealed_ink;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // Shadow, WindowText, Text, ButtonText, ToolTipText and any later text role.
        return colors.pencil;
    }
}

QPalette invisible_ink_palette(FillStyle fill_style) {
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

Theme invisible_ink_theme(FillStyle fill_style) {
    return Theme(invisible_ink_palette(fill_style), QFont(hand_font_family()), RoughOptions{}, fill_style,
                 Medium::Pencil, TextInk::Invisible);
}

}  // namespace hpq
