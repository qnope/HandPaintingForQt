#include <hpq/Theme.h>

#include <utility>

namespace hpq {

Theme::Theme(QPalette palette, QFont font, RoughOptions rough_options, FillStyle fill_style)
    : m_palette(std::move(palette)),
      m_font(std::move(font)),
      m_rough_options(rough_options),
      m_fill_style(fill_style) {}

QPalette Theme::palette() const {
    return m_palette;
}

QFont Theme::font() const {
    return m_font;
}

RoughOptions Theme::rough_options() const {
    return m_rough_options;
}

FillStyle Theme::fill_style() const {
    return m_fill_style;
}

}  // namespace hpq
