#include <hpq/HandPaintStyle.h>

#include <QPainter>
#include <QStyleFactory>
#include <QStyleOption>

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

}  // namespace

// QProxyStyle takes the ownership of the base style.
HandPaintStyle::HandPaintStyle() : QProxyStyle(QStyleFactory::create(BaseStyleName)) {}

void HandPaintStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                                   const QWidget *widget) const {
    switch (element) {
    case PE_PanelButtonCommand:
        draw_button_panel(*option, *painter);
        return;
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }
}

}  // namespace hpq
