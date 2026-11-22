#include <hpq/HandPaintEngine.h>

#include <hpq/HandPaintDevice.h>

#include <QPen>
#include <QPolygonF>
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

void HandPaintEngine::updateState(const QPaintEngineState& state)
{
    if (state.state() & DirtyPen)
        m_has_pen = state.pen().style() != Qt::NoPen;
    if (state.state() & DirtyBrush)
        m_has_brush = state.brush().style() != Qt::NoBrush;
}

void HandPaintEngine::drawPath(const QPainterPath& path) { draw_hand_drawn(path, true); }

void HandPaintEngine::drawEllipse(const QRectF& rect)
{
    QPainterPath ideal;
    ideal.addEllipse(rect);
    draw_hand_drawn(ideal, true);
}

void HandPaintEngine::drawPolygon(const QPointF* points, int point_count, PolygonDrawMode mode)
{
    QPainterPath ideal;
    ideal.addPolygon(QPolygonF(QVector<QPointF>(points, points + point_count)));
    const bool closed = mode != PolylineMode;
    if (closed)
        ideal.closeSubpath();
    ideal.setFillRule(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    draw_hand_drawn(ideal, closed);
}

void HandPaintEngine::draw_hand_drawn(const QPainterPath& ideal, bool filled_with_brush)
{
    const Theme& theme = m_device.theme();
    const HandDrawnShape shape = m_device.geometry_engine().hand_draw(ideal, theme, m_random);
    if (filled_with_brush && m_has_brush)
        m_painter.fillPath(shape.fill_area, theme.fill());
    if (m_has_pen)
        m_painter.strokePath(shape.strokes, QPen(theme.ink(), theme.stroke_width()));
}

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
