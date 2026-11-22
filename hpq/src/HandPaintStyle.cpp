#include <hpq/HandPaintStyle.h>

#include <QAbstractSpinBox>
#include <QPainter>
#include <QPolygonF>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>

#include <algorithm>

namespace hpq {

namespace {

const QString BaseStyleName("Fusion");
// Thickness, in pixels, of the line along which the slider handle moves.
constexpr qreal SliderTrackThickness = 4.0;

// Fusion may place the handle 1 px outside the slider: fit it across the slider and centre it,
// so that its whole outline is visible.
QRectF fitted_handle(const QRect &handle, const QStyleOptionSlider &option) {
    QRectF fitted = handle;
    const QRectF bounds = option.rect;
    if (option.orientation == Qt::Horizontal) {
        fitted.setHeight(std::min(fitted.height(), bounds.height()));
        fitted.moveTop(bounds.center().y() - fitted.height() / 2);
    } else {
        fitted.setWidth(std::min(fitted.width(), bounds.width()));
        fitted.moveLeft(bounds.center().x() - fitted.width() / 2);
    }
    return fitted;
}

// Flat push button: Button fill when raised, Midlight fill when sunken or on, Dark outline.
void draw_button_panel(const QStyleOption &option, QPainter &painter) {
    const bool is_pressed = option.state & (QStyle::State_Sunken | QStyle::State_On);
    const QPalette::ColorRole fill_role = is_pressed ? QPalette::Midlight : QPalette::Button;

    painter.save();
    painter.setBrush(option.palette.color(fill_role));
    painter.setPen(option.palette.color(QPalette::Dark));
    painter.drawRect(option.rect.adjusted(0, 0, -1, -1));
    painter.restore();
}

// The triangle of an arrow, centered in the rectangle, whose tip points to the given side.
QPolygonF arrow_triangle(QStyle::PrimitiveElement arrow, const QRectF &rect) {
    const qreal half_size = std::min(rect.width(), rect.height()) / 4;
    const QPointF center = rect.center();
    const qreal left = center.x() - half_size;
    const qreal right = center.x() + half_size;
    const qreal top = center.y() - half_size;
    const qreal bottom = center.y() + half_size;
    switch (arrow) {
    case QStyle::PE_IndicatorArrowUp:
        return QPolygonF({{center.x(), top}, {right, bottom}, {left, bottom}});
    case QStyle::PE_IndicatorArrowDown:
        return QPolygonF({{center.x(), bottom}, {left, top}, {right, top}});
    case QStyle::PE_IndicatorArrowLeft:
        return QPolygonF({{left, center.y()}, {right, top}, {right, bottom}});
    default:
        return QPolygonF({{right, center.y()}, {left, bottom}, {left, top}});
    }
}

// One filled triangle in the ButtonText color, no pixmap.
void draw_arrow(QStyle::PrimitiveElement arrow, const QStyleOption &option, QPainter &painter) {
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(option.palette.color(QPalette::ButtonText));
    painter.drawPolygon(arrow_triangle(arrow, option.rect));
    painter.restore();
}

// One rectangle outline inside the option rectangle, in the palette Dark color, without fill.
void draw_group_box_frame(const QStyleOption &option, QPainter &painter) {
    painter.save();
    painter.setPen(option.palette.color(QPalette::Dark));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(option.rect.adjusted(0, 0, -1, -1));
    painter.restore();
}

// A header section as a flat rectangle: Button fill, Dark outline, no cached pixmap.
void draw_header_section(const QStyleOption &option, QPainter &painter) {
    painter.save();
    painter.setPen(option.palette.color(QPalette::Dark));
    painter.setBrush(option.palette.color(QPalette::Button));
    painter.drawRect(option.rect.adjusted(0, 0, -1, -1));
    painter.restore();
}

// A flat spin box: Base fill with a Dark outline, and the up and down arrows of the style
// in their buttons, instead of Fusion's cached pixmap.
void draw_spin_box(const QStyle &style, const QStyleOptionSpinBox &option, QPainter &painter,
                   const QWidget *widget) {
    painter.save();
    painter.setPen(option.frame ? QPen(option.palette.color(QPalette::Dark)) : QPen(Qt::NoPen));
    painter.setBrush(option.palette.color(QPalette::Base));
    painter.drawRect(option.rect.adjusted(0, 0, -1, -1));
    painter.restore();

    if (option.buttonSymbols == QAbstractSpinBox::NoButtons) {
        return;
    }
    QStyleOption arrow = option;
    arrow.rect = style.subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, widget);
    style.drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrow, &painter, widget);
    arrow.rect = style.subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, widget);
    style.drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, widget);
}

}  // namespace

// QProxyStyle takes the ownership of the base style.
HandPaintStyle::HandPaintStyle() : QProxyStyle(QStyleFactory::create(BaseStyleName)) {}

void HandPaintStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget) const {
    if (element == CE_HeaderSection) {
        draw_header_section(*option, *painter);
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void HandPaintStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                                   const QWidget *widget) const {
    switch (element) {
    case PE_PanelButtonCommand:
        draw_button_panel(*option, *painter);
        return;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
        draw_arrow(element, *option, *painter);
        return;
    case PE_FrameGroupBox:
        draw_group_box_frame(*option, *painter);
        return;
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }
}

void HandPaintStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                        QPainter *painter, const QWidget *widget) const {
    if (control == CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            draw_slider(*slider, *painter, widget);
            return;
        }
    }
    if (control == CC_ScrollBar) {
        if (const auto *scroll_bar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            draw_scroll_bar(*scroll_bar, *painter, widget);
            return;
        }
    }
    if (control == CC_SpinBox) {
        if (const auto *spin_box = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            draw_spin_box(*this, *spin_box, *painter, widget);
            return;
        }
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

// Fusion caches the groove and the handle in pixmaps: draw them as flat shapes instead,
// and let Fusion draw the tick marks, which it draws with lines.
void HandPaintStyle::draw_slider(const QStyleOptionSlider &option, QPainter &painter,
                                 const QWidget *widget) const {
    const QRect groove = subControlRect(CC_Slider, &option, SC_SliderGroove, widget);
    const QRect handle = subControlRect(CC_Slider, &option, SC_SliderHandle, widget);
    const QPalette &palette = option.palette;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(palette.color(QPalette::Dark));
    if (option.subControls & SC_SliderGroove) {
        QRectF track = groove;
        if (option.orientation == Qt::Horizontal) {
            track.setHeight(SliderTrackThickness);
        } else {
            track.setWidth(SliderTrackThickness);
        }
        track.moveCenter(QRectF(groove).center());
        painter.setBrush(palette.color(QPalette::Mid));
        painter.drawRect(track);
    }
    if (option.subControls & SC_SliderHandle) {
        painter.setBrush(palette.color(QPalette::Button));
        painter.drawRect(fitted_handle(handle, option).adjusted(0.5, 0.5, -0.5, -0.5));
    }
    painter.restore();

    if (option.subControls & SC_SliderTickmarks) {
        QStyleOptionSlider tick_marks = option;
        tick_marks.subControls = SC_SliderTickmarks;
        QProxyStyle::drawComplexControl(CC_Slider, &tick_marks, &painter, widget);
    }
}

// Fusion draws the arrows of the line buttons from a cached pixmap: let Fusion draw the rest,
// then draw each line button as a flat panel with a vector arrow.
void HandPaintStyle::draw_scroll_bar(const QStyleOptionSlider &option, QPainter &painter,
                                     const QWidget *widget) const {
    QStyleOptionSlider without_line_buttons = option;
    without_line_buttons.subControls &= ~(SC_ScrollBarSubLine | SC_ScrollBarAddLine);
    QProxyStyle::drawComplexControl(CC_ScrollBar, &without_line_buttons, &painter, widget);

    const bool is_horizontal = option.orientation == Qt::Horizontal;
    const PrimitiveElement sub_arrow = is_horizontal ? PE_IndicatorArrowLeft : PE_IndicatorArrowUp;
    const PrimitiveElement add_arrow = is_horizontal ? PE_IndicatorArrowRight : PE_IndicatorArrowDown;
    draw_scroll_bar_line_button(option, SC_ScrollBarSubLine, sub_arrow, painter, widget);
    draw_scroll_bar_line_button(option, SC_ScrollBarAddLine, add_arrow, painter, widget);
}

void HandPaintStyle::draw_scroll_bar_line_button(const QStyleOptionSlider &option, SubControl button,
                                                 PrimitiveElement arrow, QPainter &painter,
                                                 const QWidget *widget) const {
    if (!(option.subControls & button)) {
        return;
    }
    QStyleOption button_option = option;
    button_option.rect = subControlRect(CC_ScrollBar, &option, button, widget);
    const bool is_pressed = (option.activeSubControls & button) && (option.state & State_Sunken);
    button_option.state &= ~(State_Sunken | State_On);
    if (is_pressed) {
        button_option.state |= State_Sunken;
    }
    drawPrimitive(PE_PanelButtonCommand, &button_option, &painter, widget);
    drawPrimitive(arrow, &button_option, &painter, widget);
}

}  // namespace hpq
