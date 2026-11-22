#include <hpq/HandPaintStyle.h>

#include <QPainter>
#include <QPolygonF>
#include <QStyleFactory>
#include <QStyleOption>

#include <algorithm>

namespace hpq {

namespace {

const QString BaseStyleName("Fusion");

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

}  // namespace

// QProxyStyle takes the ownership of the base style.
HandPaintStyle::HandPaintStyle() : QProxyStyle(QStyleFactory::create(BaseStyleName)) {}

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
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }
}

}  // namespace hpq
