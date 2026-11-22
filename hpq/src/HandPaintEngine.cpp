#include <hpq/HandPaintEngine.h>

#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include <QPainter>

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

// Images reach the target unchanged; not drawn yet.
void HandPaintEngine::drawPixmap(const QRectF &, const QPixmap &, const QRectF &) {}

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

}  // namespace hpq
