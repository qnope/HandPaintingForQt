#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/GeometryEngine.h>

#include <QPaintEngine>
#include <QPainter>

namespace hpq {

// The paint engine behind a HandPaintDevice: it paints, hand-drawn, on the device target.
class HandPaintEngine : public QPaintEngine
{
public:
    explicit HandPaintEngine(const HandPaintDevice& device);

    bool begin(QPaintDevice* device) override;
    bool end() override;
    void updateState(const QPaintEngineState& state) override;

    void drawPath(const QPainterPath& path) override;
    void drawPolygon(const QPointF* points, int point_count, PolygonDrawMode mode) override;
    void drawPixmap(const QRectF& target_rect, const QPixmap& pixmap, const QRectF& source_rect) override;
    void drawImage(const QRectF& target_rect, const QImage& image, const QRectF& source_rect,
                   Qt::ImageConversionFlags flags) override;
    void drawTextItem(const QPointF& position, const QTextItem& text_item) override;

    Type type() const override;

private:
    // Draws ideal hand-drawn on the target; filled_with_brush tells whether the brush fills it.
    void draw_hand_drawn(const QPainterPath& ideal, bool filled_with_brush);

    const HandPaintDevice& m_device;
    QPainter m_painter;
    RandomGenerator m_random;
    bool m_has_pen = true;
    bool m_has_brush = false;
};

} // namespace hpq
