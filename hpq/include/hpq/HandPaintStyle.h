#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>
#include <hpq/Theme.h>

#include <QProxyStyle>

class QLabel;
class QStyleOptionComboBox;
class QStyleOptionSlider;

namespace hpq {

// Fusion, with the parts Fusion draws as images drawn with flat vector primitives instead,
// so that a HandPaintEngine can make them rough.
class HPQ_EXPORT HandPaintStyle : public QProxyStyle {
    Q_OBJECT

public:
    // The dynamic property of a QLabel that holds the QBrush of its marker. A label that has
    // it gets one line of that brush under each line of its text, as high as the text and a
    // little longer; the line of a disabled label is lighter.
    static constexpr char LabelMarkerProperty[] = "hpq_label_marker";

    // The entries of a menu (as the list of a combo box) leave room for the hand-drawn
    // highlight of `theme`, which goes past its rectangle.
    explicit HandPaintStyle(Theme theme);

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const override;

    // An entry of a menu is larger by the highlight overflow of the theme on each side.
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget = nullptr) const override;

    // The grid lines of a table are between the Window and the Mid colors of the palette:
    // lighter than a dark window, where Fusion only darkens the Window color.
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *return_data = nullptr) const override;

private:
    int highlight_overflow() const;
    void draw_label_marker(const QLabel &label, QPainter &painter) const;
    void draw_slider(const QStyleOptionSlider &option, QPainter &painter, const QWidget *widget) const;
    void draw_combo_box(const QStyleOptionComboBox &option, QPainter &painter, const QWidget *widget) const;
    void draw_scroll_bar(const QStyleOptionSlider &option, QPainter &painter, const QWidget *widget) const;
    void draw_scroll_bar_line_button(const QStyleOptionSlider &option, SubControl button, PrimitiveElement arrow,
                                     QPainter &painter, const QWidget *widget) const;

    Theme m_theme;
};

}  // namespace hpq
