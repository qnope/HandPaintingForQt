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

void HandPaintEngine::drawPixmap(const QRectF& target_rect, const QPixmap& pixmap, const QRectF& source_rect)
{
    m_painter.drawPixmap(target_rect, pixmap, source_rect);
}

void HandPaintEngine::drawImage(const QRectF& target_rect, const QImage& image, const QRectF& source_rect,
                                Qt::ImageConversionFlags flags)
{
    m_painter.drawImage(target_rect, image, source_rect, flags);
}

// Text keeps its shape: it is drawn unchanged, only in the theme ink color.
void HandPaintEngine::drawTextItem(const QPointF& position, const QTextItem& text_item)
{
    m_painter.setPen(m_device.theme().ink());
    m_painter.setFont(text_item.font());
    m_painter.drawText(position, text_item.text());
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

} // namespace hpq
