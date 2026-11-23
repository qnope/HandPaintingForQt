#pragma once

#include <hpq/FillStyle.h>
#include <hpq/FwdDecls.h>
#include <hpq/Medium.h>
#include <hpq/RoughOptions.h>
#include <hpq/TextInk.h>

#include <QBrush>
#include <QFont>
#include <QPalette>

namespace hpq {

// What a hand-painted look draws with: colors, font, roughness, fill, medium, the ink of its
// text and the marker of its labels.
// The values are set at construction and can only be read afterwards.
class Theme {
public:
    Theme(QPalette palette, QFont font, RoughOptions rough_options, FillStyle fill_style,
          Medium medium = Medium::Pencil, TextInk text_ink = TextInk::Visible, QBrush label_marker = Qt::NoBrush);

    QPalette palette() const;
    QFont font() const;
    RoughOptions rough_options() const;
    FillStyle fill_style() const;
    Medium medium() const;
    TextInk text_ink() const;
    // What the theme marks the text of its labels with: one line as high as the text, drawn
    // under it. Qt::NoBrush marks nothing.
    QBrush label_marker() const;

private:
    QPalette m_palette;
    QFont m_font;
    RoughOptions m_rough_options;
    FillStyle m_fill_style;
    Medium m_medium;
    TextInk m_text_ink;
    QBrush m_label_marker;
};

}  // namespace hpq
