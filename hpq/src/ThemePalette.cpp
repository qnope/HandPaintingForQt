#include "ThemePalette.h"

namespace hpq {

namespace {

const QBrush &brush_for(QPalette::ColorRole role, const PaletteColors &colors) {
    switch (role) {
    case QPalette::Window:
        return colors.window;
    case QPalette::Base:
        return colors.base;
    case QPalette::Button:
        return colors.button;
    case QPalette::Light:
        return colors.light;
    case QPalette::BrightText:
        return colors.bright_text;
    case QPalette::ToolTipBase:
        return colors.tool_tip_base;
    case QPalette::AlternateBase:
        return colors.alternate_base;
    case QPalette::Midlight:
        return colors.midlight;
    case QPalette::Mid:
        return colors.mid;
    case QPalette::Dark:
        return colors.dark;
    case QPalette::Shadow:
        return colors.shadow;
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
        return colors.placeholder_text;
    default:  // WindowText, Text, ButtonText, ToolTipText and any later text role.
        return colors.text;
    }
}

}  // namespace

PaletteColors::PaletteColors(const QBrush &ground, const QBrush &text)
    : window(ground),
      base(ground),
      button(ground),
      light(ground),
      bright_text(ground),
      tool_tip_base(ground),
      alternate_base(ground),
      midlight(ground),
      mid(text),
      dark(text),
      shadow(text),
      highlight(text),
      highlighted_text(text),
      link(text),
      placeholder_text(text),
      text(text) {}

QPalette make_palette(const PaletteColors &enabled, const PaletteColors &disabled) {
    QPalette palette;
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const PaletteColors &colors = group == QPalette::Disabled ? disabled : enabled;
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto color_role = static_cast<QPalette::ColorRole>(role);
            palette.setBrush(group, color_role, brush_for(color_role, colors));
        }
    }
    return palette;
}

}  // namespace hpq
