#include <hpq/Themes.h>

#include "Fonts.h"
#include "ThemePalette.h"

#include <QColor>

namespace hpq {

namespace {

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

// What is disabled is written with a pencil, and marked by highlighters that have run dry.
PaletteColors highlighter_colors(bool is_disabled, FillStyle fill_style) {
    const QColor paper(0xff, 0xff, 0xff);
    const QColor pen = is_disabled ? QColor(0xa6, 0xa8, 0xae) : QColor(0x1a, 0x1a, 0x20);
    const QColor pencil = is_disabled ? QColor(0xc4, 0xc6, 0xcc) : QColor(0x8e, 0x90, 0x98);
    const QColor yellow = is_disabled ? DryYellow : Yellow;
    const QColor green = is_disabled ? DryGreen : Green;
    const QColor pink = is_disabled ? DryPink : Pink;
    const QColor orange = is_disabled ? DryOrange : Orange;
    // Only an opaque fill is hatched.
    const bool is_hatched = fill_style == FillStyle::Hachure;

    PaletteColors colors(paper, pen);
    colors.button = is_hatched ? green : passed(green, OnePass);
    // A pressed button.
    colors.midlight = is_hatched ? green : passed(green, TwoPasses);
    colors.alternate_base = is_hatched ? PaleBlue : passed(Blue, LightPass);
    colors.tool_tip_base = QColor(0xfd, 0xff, 0x8c);  // A sticky note.
    colors.mid = pencil;
    // The frames.
    colors.dark = passed(pink, OnePass);
    // The thin lines of the yellow highlighter would not show on the paper: the orange one
    // hatches the selection.
    colors.highlight = is_hatched ? orange : passed(yellow, TwoPasses);
    // A pink felt pen.
    colors.link = is_disabled ? QColor(0xe0, 0xa8, 0xc6) : QColor(0xd9, 0x1a, 0x7c);
    colors.placeholder_text = QColor(0x9a, 0x9c, 0xa4);
    return colors;
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
    Theme theme;
    theme.palette = make_palette(highlighter_colors(false, fill_style), highlighter_colors(true, fill_style));
    theme.font = QFont(hand_font_family());
    theme.rough_options = HighlighterRoughOptions;
    theme.fill_style = fill_style;
    theme.label_marker = label_marker();
    return theme;
}

}  // namespace hpq
