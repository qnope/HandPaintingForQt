#include <hpq/Theme.h>

#include <utility>

namespace hpq {

Theme::Theme(QPalette palette, QFont font, RoughOptions rough_options, FillStyle fill_style, Medium medium,
             TextInk text_ink, QBrush label_marker)
    : m_palette(std::move(palette)),
      m_font(std::move(font)),
      m_rough_options(rough_options),
      m_fill_style(fill_style),
      m_medium(medium),
      m_text_ink(text_ink),
      m_label_marker(std::move(label_marker)) {}

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

Medium Theme::medium() const {
    return m_medium;
}

TextInk Theme::text_ink() const {
    return m_text_ink;
}

QBrush Theme::label_marker() const {
    return m_label_marker;
}

AntialiasingMode Theme::antialiasing_mode() const {
    return m_antialiasing_mode;
}

Theme Theme::with_antialiasing_mode(AntialiasingMode mode) const {
    Theme theme = *this;
    theme.m_antialiasing_mode = mode;
    return theme;
}

}  // namespace hpq
