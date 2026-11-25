#include <hpq/ChinaInkTheme.h>

#include "EmbeddedFont.h"

#include <QColor>
#include <QtGlobal>

// Where hpq is a static library (Qt for WebAssembly), its resource must be initialized by
// hand, outside any namespace. In the shared library, the call does nothing more.
static void init_hpq_china_ink_fonts_resource() {
    Q_INIT_RESOURCE(hpq_china_ink_fonts);
}

namespace hpq {

namespace {

// The family of the embedded brush font, registered in QFontDatabase by the first call made
// once the QGuiApplication exists.
QString brush_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/KaushanScript-Regular.ttf"),
                                init_hpq_china_ink_fonts_resource);
}

// One color for each role of a color group.
struct GroupColors {
    QColor paper;
    QColor wash;
    QColor ink;
    QColor ink_line;
    QColor ink_pale;
    QColor seal;
    QColor seal_text;
    QColor link;
    QColor placeholder;
};

constexpr QPalette::ColorGroup PaletteGroups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

GroupColors colors_for(QPalette::ColorGroup group, FillStyle fill_style) {
    GroupColors colors{
        QColor(0xf3, 0xec, 0xda),  // paper
        QColor(0xdc, 0xd7, 0xc9),  // wash
        QColor(0x1b, 0x1a, 0x1c),  // ink
        QColor(0x2e, 0x2c, 0x2a),  // ink_line
        QColor(0x8d, 0x88, 0x7c),  // ink_pale
        QColor(0xb8, 0x32, 0x2a),  // seal
        QColor(0xf3, 0xec, 0xda),  // seal_text
        QColor(0x2c, 0x4a, 0x6e),  // link
        QColor(0x9c, 0x97, 0x8a),  // placeholder
    };
    if (group == QPalette::Disabled) {
        colors.ink = QColor(0xa9, 0xa4, 0x98);
        colors.ink_line = QColor(0xa9, 0xa4, 0x98);
        colors.seal = QColor(0xcd, 0xb9, 0xb0);
        colors.link = QColor(0x9a, 0xa4, 0xb4);
    }
    // The paper shows between the hachures of a selection: its text is read on the paper,
    // through hachures of a red diluted enough not to hide it.
    if (fill_style == FillStyle::Hachure) {
        colors.seal_text = colors.ink;
        colors.seal = group == QPalette::Disabled ? QColor(0xdc, 0xc8, 0xc0) : QColor(0xcc, 0x55, 0x48);
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
        return colors.wash;
    case QPalette::Mid:
        return colors.ink_pale;
    case QPalette::Dark:
        return colors.ink_line;
    case QPalette::Highlight:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)  // The accent role appeared in Qt 6.6.
    case QPalette::Accent:
#endif
        return colors.seal;
    case QPalette::HighlightedText:
        return colors.seal_text;
    case QPalette::Link:
    case QPalette::LinkVisited:
        return colors.link;
    case QPalette::PlaceholderText:
        return colors.placeholder;
    default:  // WindowText, Text, ButtonText, ToolTipText, Shadow and any later text role.
        return colors.ink;
    }
}

QPalette china_ink_palette(FillStyle fill_style) {
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

// A brush draws a line in one stroke. Its hachures are wider apart than those of a pencil.
constexpr RoughOptions BrushRoughOptions{1.0, 1, 1.5, 5.0};

}  // namespace

Theme china_ink_theme(FillStyle fill_style) {
    return Theme(china_ink_palette(fill_style), QFont(brush_font_family()), BrushRoughOptions, fill_style,
                 Medium::Ink);
}

}  // namespace hpq
