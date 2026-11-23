#include <hpq/NotebookTheme.h>

#include "SchoolFont.h"
#include "SeyesPaper.h"

#include <QBrush>
#include <QColor>

namespace hpq {

namespace {

// One brush or color for each role of a color group.
struct GroupColors {
    QBrush sheet;
    QColor paper;
    QColor see_through;
    QColor wash;
    QColor ink;
    QColor ink_light;
    QColor ink_pale;
    QColor highlight;
    QColor link;
    QColor placeholder;
    QColor sticky_note;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

constexpr QColor Paper(0xfd, 0xfd, 0xf9);
constexpr QColor Pencil(0x9c, 0x9c, 0xa2);
constexpr QColor PencilLight(0xbc, 0xbc, 0xc2);

// The ruled sheet as a brush of the color of the paper. Every theme shares one image: two
// brushes are equal only when they share it, and so are the palettes of two themes.
QBrush sheet_brush() {
    static const QImage sheet = seyes_sheet({
        Paper,
        QColor(0xb4, 0xbe, 0xea),  // line
        QColor(0xdd, 0xe2, 0xf6),  // fine_line
        QColor(0xf0, 0x86, 0x94),  // margin
    });
    QBrush brush(sheet);
    // The image has its own colors: the color of the brush tells what the styles take for it.
    brush.setColor(Paper);
    return brush;
}

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        sheet_brush(),                   // sheet
        Paper,                           // paper
        QColor(0xfd, 0xfd, 0xf9, 0),     // see_through
        QColor(0x2b, 0x4a, 0xc0, 0x26),  // wash
        QColor(0x1d, 0x35, 0x9a),        // ink
        QColor(0x4f, 0x69, 0xc6),        // ink_light
        QColor(0x93, 0xa4, 0xde),        // ink_pale
        QColor(0xff, 0xe0, 0x2a, 0xc0),  // highlight
        QColor(0xcf, 0x2a, 0x2a),        // link
        Pencil,                          // placeholder
        QColor(0xff, 0xf4, 0x9c),        // sticky_note
    };
    // What is disabled is written with a pencil.
    if (group == QPalette::Disabled) {
        colors.ink = Pencil;
        colors.ink_light = PencilLight;
        colors.ink_pale = PencilLight;
        colors.highlight = QColor(0xd9, 0xd9, 0xd4, 0xb4);
        colors.link = QColor(0xd8, 0xa9, 0xa9);
        colors.wash = QColor(0x80, 0x80, 0x88, 0x1c);
    }
    // A hatched fill is opaque: the highlighter gives way to a colored pencil, whose thin lines
    // show on the paper.
    if (fill_style == FillStyle::Hachure) {
        colors.highlight = group == QPalette::Disabled ? PencilLight : QColor(0xf2, 0xa9, 0x00);
    }
    return colors;
}

QBrush brush_for(QPalette::ColorRole role, const GroupColors &colors) {
    switch (role) {
    case QPalette::Window:
        return colors.sheet;
    case QPalette::Base:
    case QPalette::Button:
        return colors.see_through;
    case QPalette::Light:
    case QPalette::BrightText:
        return colors.paper;
    case QPalette::ToolTipBase:
        return colors.sticky_note;
    case QPalette::AlternateBase:
    case QPalette::Midlight:
        return colors.wash;
    case QPalette::Mid:
        return colors.ink_pale;
    case QPalette::Dark:
        return colors.ink_light;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.highlight;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // WindowText, Text, ButtonText, HighlightedText, ToolTipText, Shadow and any later text role.
        return colors.ink;
    }
}

QPalette notebook_palette(FillStyle fill_style) {
    QPalette palette;
    for (const QPalette::ColorGroup group : PaletteGroups) {
        const GroupColors colors = colors_for(group, fill_style);
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            palette.setBrush(group, color_role, brush_for(color_role, colors));
        }
    }
    return palette;
}

// A pen draws one line, and a pupil draws it with care.
constexpr RoughOptions PenRoughOptions{0.8, 1, 1.5};

// The school handwriting has tall capitals and long ascenders: at this fraction of the size of
// the application font, its small letters are as high as those of the application font would be.
constexpr double HandwritingScale = 0.85;

// The school font, at the size of the application font scaled by HandwritingScale.
QFont handwriting_font() {
    const QFont application_font;
    QFont font(school_font_family());
    if (application_font.pointSizeF() > 0) {
        font.setPointSizeF(application_font.pointSizeF() * HandwritingScale);
    } else if (application_font.pixelSize() > 0) {
        font.setPixelSize(qRound(application_font.pixelSize() * HandwritingScale));
    }
    return font;
}

}  // namespace

Theme notebook_theme(FillStyle fill_style) {
    return Theme(notebook_palette(fill_style), handwriting_font(), PenRoughOptions, fill_style);
}

}  // namespace hpq
