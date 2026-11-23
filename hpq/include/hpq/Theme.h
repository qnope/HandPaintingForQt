#pragma once

#include <hpq/FillStyle.h>
#include <hpq/FwdDecls.h>
#include <hpq/Medium.h>
#include <hpq/RoughOptions.h>
#include <hpq/TextInk.h>

#include <QFont>
#include <QPalette>

namespace hpq {

// What a hand-painted look draws with: colors, font, roughness, fill, medium and the ink of
// its text.
// The values are set at construction and can only be read afterwards.
class Theme {
public:
    Theme(QPalette palette, QFont font, RoughOptions rough_options, FillStyle fill_style,
          Medium medium = Medium::Pencil, TextInk text_ink = TextInk::Visible);

    QPalette palette() const;
    QFont font() const;
    RoughOptions rough_options() const;
    FillStyle fill_style() const;
    Medium medium() const;
    TextInk text_ink() const;

private:
    QPalette m_palette;
    QFont m_font;
    RoughOptions m_rough_options;
    FillStyle m_fill_style;
    Medium m_medium;
    TextInk m_text_ink;
};

}  // namespace hpq
