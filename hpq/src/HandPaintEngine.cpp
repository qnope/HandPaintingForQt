#include <hpq/HandPaintEngine.h>

#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include <QPaintDevice>
#include <QPainter>
#include <QTextItem>

#include <cmath>

namespace hpq {

HandPaintEngine::HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine)
    : QPaintEngine(QPaintEngine::AllFeatures), m_target(target), m_theme(theme), m_geometry_engine(geometry_engine) {}

bool HandPaintEngine::begin(QPaintDevice *) { return true; }

bool HandPaintEngine::end() { return true; }

void HandPaintEngine::updateState(const QPaintEngineState &state) {
    const DirtyFlags flags = state.state();
    if (flags & DirtyPen) {
        m_pen = state.pen();
    }
    if (flags & DirtyBrush) {
        m_brush = state.brush();
    }
}

void HandPaintEngine::drawPath(const QPainterPath &path) { draw_shape(path, true); }

void HandPaintEngine::drawRects(const QRectF *rects, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape;
        shape.addRect(rects[index]);
        draw_shape(shape, true);
    }
}

void HandPaintEngine::drawLines(const QLineF *lines, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape(lines[index].p1());
        shape.lineTo(lines[index].p2());
        draw_shape(shape, false);
    }
}

void HandPaintEngine::drawEllipse(const QRectF &rect) {
    QPainterPath shape;
    shape.addEllipse(rect);
    draw_shape(shape, true);
}

// A polyline is an open shape; the other modes are closed and keep their fill rule.
void HandPaintEngine::drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) {
    if (count <= 0) {
        return;
    }
    QPainterPath shape(points[0]);
    for (int index = 1; index < count; ++index) {
        shape.lineTo(points[index]);
    }
    const bool closed = mode != PolylineMode;
    if (closed) {
        shape.closeSubpath();
    }
    shape.setFillRule(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    draw_shape(shape, closed);
}

// Images reach the target unchanged.
void HandPaintEngine::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) {
    m_target.save();
    m_target.drawPixmap(rect, pixmap, source_rect);
    m_target.restore();
}

void HandPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                Qt::ImageConversionFlags flags) {
    m_target.save();
    m_target.drawImage(rect, image, source_rect, flags);
    m_target.restore();
}

void HandPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) {
    m_target.save();
    m_target.drawTiledPixmap(rect, pixmap, offset);
    m_target.restore();
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

void HandPaintEngine::draw_shape(const QPainterPath &shape, bool closed) {
    const RoughOptions options = m_theme.rough_options();
    const std::uint32_t shape_seed = seed(shape);
    if (closed && m_brush.style() != Qt::NoBrush) {
        draw_on_target(m_geometry_engine.fill_area(shape, options, shape_seed), Qt::NoPen, m_brush);
    }
    if (m_pen.style() != Qt::NoPen) {
        for (const QPainterPath &outline : m_geometry_engine.outline(shape, options, shape_seed)) {
            draw_on_target(outline, m_pen, Qt::NoBrush);
        }
    }
}

void HandPaintEngine::draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush) {
    m_target.save();
    m_target.setPen(pen);
    m_target.setBrush(brush);
    m_target.drawPath(path);
    m_target.restore();
}

// The same seed for every shape, for now.
std::uint32_t HandPaintEngine::seed(const QPainterPath &) const { return 0; }

namespace {

constexpr double PointsPerInch = 72.0;

// Pixel size of `font` on `device`: its pixel size, or its point size converted with the device DPI.
int pixel_size(const QFont &font, const QPaintDevice &device) {
    if (font.pixelSize() > 0) {
        return font.pixelSize();
    }
    return static_cast<int>(std::lround(font.pointSizeF() * device.logicalDpiY() / PointsPerInch));
}

}  // namespace

void HandPaintEngine::drawTextItem(const QPointF &position, const QTextItem &text_item) {
    QFont font = m_theme.font();
    font.setPixelSize(pixel_size(text_item.font(), *paintDevice()));
    m_target.save();
    m_target.setPen(m_pen);
    m_target.setFont(font);
    m_target.drawText(position, text_item.text());
    m_target.restore();
}

}  // namespace hpq
