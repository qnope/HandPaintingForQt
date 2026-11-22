#include <hpq/HandPaintEngine.h>

#include <hpq/HandPaintDevice.h>

#include <QRect>

namespace hpq {

HandPaintEngine::HandPaintEngine(const HandPaintDevice& device)
    : m_device(device)
{
}

bool HandPaintEngine::begin(QPaintDevice* /*device*/)
{
    QPaintDevice& target = m_device.target();
    if (!m_painter.begin(&target))
        return false;
    m_painter.setRenderHint(QPainter::Antialiasing);
    m_painter.fillRect(QRect(0, 0, target.width(), target.height()), m_device.theme().background());
    return true;
}

bool HandPaintEngine::end() { return m_painter.end(); }

void HandPaintEngine::updateState(const QPaintEngineState& /*state*/) {}

void HandPaintEngine::drawPath(const QPainterPath& /*path*/) {}

void HandPaintEngine::drawPolygon(const QPointF* /*points*/, int /*point_count*/, PolygonDrawMode /*mode*/) {}

void HandPaintEngine::drawPixmap(const QRectF& /*target_rect*/, const QPixmap& /*pixmap*/,
                                 const QRectF& /*source_rect*/)
{
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

} // namespace hpq
