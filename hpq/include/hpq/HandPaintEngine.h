#pragma once

#include <hpq/FwdDecls.h>

#include <QBrush>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>

#include <cstdint>

class QPainter;

namespace hpq {

// Paint engine that draws each primitive it receives on a target QPainter,
// changed by a GeometryEngine and drawn as a Theme says.
class HandPaintEngine : public QPaintEngine {
public:
    HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine);

    bool begin(QPaintDevice *device) override;
    bool end() override;
    void updateState(const QPaintEngineState &state) override;
    void drawPath(const QPainterPath &path) override;
    void drawRects(const QRectF *rects, int count) override;
    void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) override;
    void drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) override;
    Type type() const override;
    // Draws the text on the target in the theme font, at the same pixel size, in the pen color.
    void drawTextItem(const QPointF &position, const QTextItem &text_item) override;

private:
    // Draws `shape` on the target: its rough fill area when it is closed and the brush is set,
    // then its rough outline strokes when the pen is set.
    void draw_shape(const QPainterPath &shape, bool closed);
    // Draws `path` on the target with `pen` and `brush`, leaving the target state unchanged.
    void draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush);
    std::uint32_t seed(const QPainterPath &shape) const;

    QPainter &m_target;
    const Theme &m_theme;
    const GeometryEngine &m_geometry_engine;
    QPen m_pen;
    QBrush m_brush;
};

}  // namespace hpq
