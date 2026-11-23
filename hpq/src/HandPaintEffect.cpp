#include <hpq/HandPaintDevice.h>
#include <hpq/HandPaintEffect.h>

#include <QPainter>

#include <utility>

namespace hpq {

HandPaintEffect::HandPaintEffect(Theme theme,
                                 std::shared_ptr<const GeometryEngine> geometry_engine,
                                 QObject *parent)
    : QGraphicsEffect(parent),
      m_theme(std::move(theme)),
      m_geometry_engine(std::move(geometry_engine)) {}

Theme HandPaintEffect::theme() const {
    return m_theme;
}

std::shared_ptr<const GeometryEngine> HandPaintEffect::geometry_engine() const {
    return m_geometry_engine;
}

void HandPaintEffect::draw(QPainter *painter) {
    HandPaintDevice device(*painter, m_theme, *m_geometry_engine, &m_cache);
    QPainter hand_painter(&device);
    // The painter is not the one of the paint context, so Qt renders the source widget
    // (and its children) on it; the effect is not applied again inside this render.
    drawSource(&hand_painter);
}

}  // namespace hpq
