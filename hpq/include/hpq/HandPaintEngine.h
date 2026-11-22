#pragma once

#include <hpq/FwdDecls.h>

#include <QBrush>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

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
    // The integer overloads of QPaintEngine convert to the floating point ones below.
    using QPaintEngine::drawEllipse;
    using QPaintEngine::drawLines;
    using QPaintEngine::drawPolygon;
    void drawLines(const QLineF *lines, int count) override;
    void drawEllipse(const QRectF &rect) override;
    void drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) override;
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
    // Saves the target state, then intersects the target clip with the system clip and the
    // device painter's clip.
    // What is drawn next is in device coordinates; restore the target when done.
    // The system clip (the area of the widget being rendered) is grown by `system_clip_margin`.
    void save_target_in_device_coordinates(int system_clip_margin = 0);
    // Like save_target_in_device_coordinates, then composes the device painter's world transform,
    // so that what is drawn next is in the device painter's logical coordinates.
    void save_target_in_logical_coordinates();

    QPainter &m_target;
    const Theme &m_theme;
    const GeometryEngine &m_geometry_engine;
    QPen m_pen;
    QBrush m_brush;
    // The device painter's world transform: shapes are mapped by it to device coordinates
    // before the geometry engine deforms them, so that max_offset stays in device pixels.
    QTransform m_transform;
    // The device painter's clip, in device coordinates; used only when m_clipping is true.
    QPainterPath m_clip;
    bool m_clipping = false;
};

}  // namespace hpq
