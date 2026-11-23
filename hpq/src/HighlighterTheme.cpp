#include <hpq/HighlighterTheme.h>

#include "HandFont.h"

#include <QColor>

namespace hpq {

namespace {

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor pen;
    QColor pencil;
    QColor selection;
    QColor button;
    QColor button_pressed;
    QColor alternate_row;
    QColor frame;
    QColor link;
    QColor placeholder;
    QColor sticky_note;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

// The highlighters of the pencil case.
constexpr QColor Yellow(0xff, 0xf0, 0x00);
constexpr QColor Green(0x5d, 0xf2, 0x3a);
constexpr QColor Blue(0x2c, 0xd2, 0xff);
constexpr QColor Pink(0xff, 0x3c, 0xa6);
constexpr QColor Orange(0xff, 0x9a, 0x1a);
// The same ones when they have run dry.
constexpr QColor DryYellow(0xf2, 0xec, 0x9a);
constexpr QColor DryGreen(0xc4, 0xec, 0xb8);
constexpr QColor DryPink(0xf2, 0xc0, 0xd8);
constexpr QColor DryOrange(0xf4, 0xd6, 0xb0);
// What a light pass of the blue highlighter leaves on the paper.
constexpr QColor PaleBlue(0xb4, 0xee, 0xff);

// How opaque a highlighter is, out of 255: a second pass over the first one is more vivid,
// and a light pass only tints the paper.
constexpr int LightPass = 0x40;
constexpr int StrokePass = 0x80;
constexpr int OnePass = 0xb4;
constexpr int TwoPasses = 0xee;

QColor passed(QColor highlighter, int opacity) {
    highlighter.setAlpha(opacity);
    return highlighter;
}

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(0xff, 0xff, 0xff),   // paper
        QColor(0x1a, 0x1a, 0x20),   // pen
        QColor(0x8e, 0x90, 0x98),   // pencil
        passed(Yellow, TwoPasses),  // selection
        passed(Green, OnePass),     // button
        passed(Green, TwoPasses),   // button_pressed
        passed(Blue, LightPass),    // alternate_row
        passed(Pink, OnePass),      // frame
        QColor(0xd9, 0x1a, 0x7c),   // link: a pink felt pen
        QColor(0x9a, 0x9c, 0xa4),   // placeholder
        QColor(0xfd, 0xff, 0x8c),   // sticky_note
    };
    const bool is_disabled = group == QPalette::Disabled;
    // What is disabled is written with a pencil, and marked by highlighters that have run dry.
    if (is_disabled) {
        colors.pen = QColor(0xa6, 0xa8, 0xae);
        colors.pencil = QColor(0xc4, 0xc6, 0xcc);
        colors.selection = passed(DryYellow, TwoPasses);
        colors.button = passed(DryGreen, OnePass);
        colors.button_pressed = passed(DryGreen, TwoPasses);
        colors.frame = passed(DryPink, OnePass);
        colors.link = QColor(0xe0, 0xa8, 0xc6);
    }
    // Only an opaque fill is hatched. The thin lines of the yellow highlighter would not show
    // on the paper: the orange one marks the selection.
    if (fill_style == FillStyle::Hachure) {
        colors.selection = is_disabled ? DryOrange : Orange;
        colors.button = is_disabled ? DryGreen : Green;
        colors.button_pressed = colors.button;
        colors.alternate_row = PaleBlue;
    }
    return colors;
}

QColor color_for(QPalette::ColorRole role, const GroupColors &colors) {
    switch (role) {
    case QPalette::Window:
    case QPalette::Base:
    case QPalette::Light:
    case QPalette::BrightText:
        return colors.paper;
    case QPalette::Button:
        return colors.button;
    case QPalette::Midlight:
        return colors.button_pressed;
    case QPalette::AlternateBase:
        return colors.alternate_row;
    case QPalette::ToolTipBase:
        return colors.sticky_note;
    case QPalette::Mid:
        return colors.pencil;
    case QPalette::Dark:
        return colors.frame;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.selection;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // Shadow, WindowText, Text, ButtonText, HighlightedText, ToolTipText and any later text role.
        return colors.pen;
    }
}

QPalette highlighter_palette(FillStyle fill_style) {
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

// The tip of a highlighter glides on the paper: three strokes that stay close to each other
// make one line, more vivid where they overlap. Its hachures lean less than those of a pencil.
constexpr RoughOptions HighlighterRoughOptions{1.0, 3, 1.2, 4.0, -30.0};

// The text of a label is marked with the yellow highlighter. Each of the strokes of its line
// is a light pass: together they are as vivid as the selection.
QBrush label_marker() {
    return passed(Yellow, StrokePass);
}

}  // namespace

Theme highlighter_theme(FillStyle fill_style) {
    return Theme(highlighter_palette(fill_style), QFont(hand_font_family()), HighlighterRoughOptions, fill_style,
                 Medium::Pencil, TextInk::Visible, label_marker());
}

}  // namespace hpq
