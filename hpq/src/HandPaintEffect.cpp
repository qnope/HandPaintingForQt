#include <hpq/HandPaintDevice.h>
#include <hpq/HandPaintEffect.h>

#include <QPainter>

#include <utility>

namespace hpq {

HandPaintEffect::HandPaintEffect(std::shared_ptr<const Theme> theme,
                                 std::shared_ptr<const GeometryEngine> geometry_engine,
                                 QObject *parent)
    : QGraphicsEffect(parent),
      m_theme(std::move(theme)),
      m_geometry_engine(std::move(geometry_engine)) {}

void HandPaintEffect::draw(QPainter *painter) {
    HandPaintDevice device(*painter, *m_theme, *m_geometry_engine);
    QPainter hand_painter(&device);
    // The painter is not the one of the paint context, so Qt renders the source widget
    // (and its children) on it; the effect is not applied again inside this render.
    drawSource(&hand_painter);
}

}  // namespace hpq
