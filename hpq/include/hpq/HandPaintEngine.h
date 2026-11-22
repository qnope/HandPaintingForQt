#pragma once

#include <hpq/FwdDecls.h>

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

    Type type() const override;

private:
    const HandPaintDevice& m_device;
    QPainter m_painter;
};

} // namespace hpq
