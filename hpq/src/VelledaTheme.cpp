#include <hpq/Themes.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

// A disabled widget is a drawing half wiped off the board.
PaletteColors velleda_colors(bool is_disabled, FillStyle fill_style) {
    const QColor board(0xfc, 0xfd, 0xfd);
    // What a wiped marker leaves.
    const QColor board_smear(0xea, 0xee, 0xf2);
    const QColor black_marker = is_disabled ? QColor(0xa9, 0xad, 0xb3) : QColor(0x1b, 0x1c, 0x22);
    const QColor blue_marker = is_disabled ? QColor(0xb4, 0xc0, 0xdc) : QColor(0x1e, 0x4f, 0xc4);
    const QColor red_marker = is_disabled ? QColor(0xe2, 0xb6, 0xb6) : QColor(0xd9, 0x2b, 0x2b);
    const QColor green_marker = is_disabled ? QColor(0xa9, 0xc9, 0xb5) : QColor(0x12, 0x8a, 0x43);

    PaletteColors colors(board, black_marker);
    colors.alternate_base = colors.midlight = board_smear;
    colors.mid = colors.dark = blue_marker;
    colors.highlight = red_marker;
    colors.highlighted_text = QColor(0xff, 0xff, 0xff);
    colors.link = green_marker;
    colors.placeholder_text = QColor(0x9a, 0xa0, 0xa8);
    // The board shows between the hachures of a selection: its text is read on the board,
    // through hachures of a red light enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.highlighted_text = black_marker;
        colors.highlight = is_disabled ? QColor(0xf1, 0xd6, 0xd6) : QColor(0xee, 0x8c, 0x8c);
    }
    return colors;
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
    Theme theme;
    theme.palette = make_palette(velleda_colors(false, fill_style), velleda_colors(true, fill_style));
    theme.font = marker_font();
    theme.rough_options = MarkerRoughOptions;
    theme.fill_style = fill_style;
    return theme;
}

}  // namespace hpq
