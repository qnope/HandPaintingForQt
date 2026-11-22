#pragma once

#include <hpq/FwdDecls.h>

#include <QProxyStyle>

class QStyleOptionComboBox;
class QStyleOptionSlider;

namespace hpq {

// Fusion, with the parts Fusion draws as images drawn with flat vector primitives instead,
// so that a HandPaintEngine can make them rough.
class HandPaintStyle : public QProxyStyle {
    Q_OBJECT

public:
    HandPaintStyle();

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const override;

private:
    void draw_slider(const QStyleOptionSlider &option, QPainter &painter, const QWidget *widget) const;
    void draw_combo_box(const QStyleOptionComboBox &option, QPainter &painter, const QWidget *widget) const;
    void draw_scroll_bar(const QStyleOptionSlider &option, QPainter &painter, const QWidget *widget) const;
    void draw_scroll_bar_line_button(const QStyleOptionSlider &option, SubControl button, PrimitiveElement arrow,
                                     QPainter &painter, const QWidget *widget) const;
};

}  // namespace hpq
