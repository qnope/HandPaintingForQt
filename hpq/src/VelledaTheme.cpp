#include <hpq/VelledaTheme.h>

#include "HandFont.h"

#include <QColor>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor board;
    QColor board_smear;
    QColor black_marker;
    QColor blue_marker;
    QColor highlight;
    QColor highlighted_text;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(0xfc, 0xfd, 0xfd),  // board
        QColor(0xea, 0xee, 0xf2),  // board_smear: what a wiped marker leaves
        QColor(0x1b, 0x1c, 0x22),  // black_marker
        QColor(0x1e, 0x4f, 0xc4),  // blue_marker
        QColor(0xd9, 0x2b, 0x2b),  // highlight: the red marker
        QColor(0xff, 0xff, 0xff),  // highlighted_text
        QColor(0x12, 0x8a, 0x43),  // link: the green marker
        QColor(0x9a, 0xa0, 0xa8),  // placeholder
    };
    // A disabled widget is a drawing half wiped off the board.
    if (group == QPalette::Disabled) {
        colors.black_marker = QColor(0xa9, 0xad, 0xb3);
        colors.blue_marker = QColor(0xb4, 0xc0, 0xdc);
        colors.highlight = QColor(0xe2, 0xb6, 0xb6);
        colors.link = QColor(0xa9, 0xc9, 0xb5);
    }
    // The board shows between the hachures of a selection: its text is read on the board,
    // through hachures of a red light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = colors.black_marker;
        colors.highlight = group == QPalette::Disabled ? QColor(0xf1, 0xd6, 0xd6) : QColor(0xee, 0x8c, 0x8c);
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
        return colors.board;
    case QPalette::AlternateBase:
    case QPalette::Midlight:
        return colors.board_smear;
    case QPalette::Mid:
    case QPalette::Dark:
        return colors.blue_marker;
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
    default:  // Shadow, WindowText, Text, ButtonText, ToolTipText and any later text role.
        return colors.black_marker;
    }
}

QPalette velleda_palette(FillStyle fill_style) {
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

// A marker glides on the board: its line wobbles less than a pencil line. Three strokes that
// stay close to each other make one thick line, with the streaks of a felt tip. Its hachures
// are wide apart.
constexpr RoughOptions MarkerRoughOptions{1.0, 3, 0.9, 7.0, -45.0};

// A felt tip writes thick letters.
QFont marker_font() {
    QFont font(hand_font_family());
    font.setWeight(QFont::Bold);
    return font;
}

}  // namespace

Theme velleda_theme(FillStyle fill_style) {
    return Theme(velleda_palette(fill_style), marker_font(), MarkerRoughOptions, fill_style);
}

}  // namespace hpq
