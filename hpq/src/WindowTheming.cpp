#include <hpq/Theming.h>
#include <hpq/WindowTheming.h>

#include <QEvent>
#include <QWidget>

#include <utility>

namespace hpq {

WindowTheming::WindowTheming(Theme theme, std::shared_ptr<const GeometryEngine> geometry_engine, QObject *parent)
    : QObject(parent), m_theme(std::move(theme)), m_geometry_engine(std::move(geometry_engine)) {}

bool WindowTheming::eventFilter(QObject *watched, QEvent *event) {
    if ((event->type() == QEvent::Polish || event->type() == QEvent::Show) && watched->isWidgetType()) {
        auto &widget = static_cast<QWidget &>(*watched);
        if (widget.isWindow() && widget.windowType() != Qt::Window) {
            apply_theme(widget, m_theme, m_geometry_engine);
        }
    }
    return false;
}

}  // namespace hpq
