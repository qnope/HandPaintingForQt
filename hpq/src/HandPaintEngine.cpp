#include <hpq/HandPaintEngine.h>

#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include <QPaintDevice>
#include <QPainter>
#include <QRegion>
#include <QTextItem>

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace hpq {

namespace {

// Coordinates are rounded to this fraction of a device pixel before they make the seed.
constexpr double SeedPrecision = 100.0;

// `region` with each of its rectangles grown by `margin` on every side.
QRegion grown(const QRegion &region, int margin) {
    if (margin == 0) {
        return region;
    }
    QRegion result;
    for (const QRect &rect : region) {
        result += rect.adjusted(-margin, -margin, margin, margin);
    }
    return result;
}

// `outline` kept out of `frame`: it follows the edge of `frame` where it would enter it.
QPainterPath kept_outside(QPainterPath outline, const QPainterPath &frame) {
    outline.setFillRule(frame.fillRule());
    return outline.united(frame);
}

std::uint32_t rounded(double coordinate) {
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::llround(coordinate * SeedPrecision)));
}

// A seed that std::seed_seq makes from the element types and rounded coordinates of `path`.
std::uint32_t path_seed(const QPainterPath &path) {
    std::vector<std::uint32_t> values;
    values.reserve(3 * static_cast<std::size_t>(path.elementCount()));
    for (int index = 0; index < path.elementCount(); ++index) {
        const QPainterPath::Element element = path.elementAt(index);
        values.push_back(static_cast<std::uint32_t>(element.type));
        values.push_back(rounded(element.x));
        values.push_back(rounded(element.y));
    }
    std::seed_seq seeds(values.begin(), values.end());
    std::uint32_t seed = 0;
    seeds.generate(&seed, &seed + 1);
    return seed;
}

}  // namespace

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
    if (flags & DirtyTransform) {
        m_transform = state.transform();
    }
    if (flags & (DirtyClipPath | DirtyClipRegion | DirtyClipEnabled)) {
        // QPainter::clipPath() is in the current logical coordinates: map it back to the device
        // with the engine's transform. Unlike QPainter::combinedTransform(), it includes the
        // redirection offset of a child widget rendered through a shared painter.
        const QPainter &painter = *state.painter();
        m_clipping = painter.hasClipping();
        m_clip = m_clipping ? state.transform().map(painter.clipPath()) : QPainterPath();
    }
}

void HandPaintEngine::drawPath(const QPainterPath &path) { draw_shape(path, ShapeKind::Closed); }

void HandPaintEngine::drawRects(const QRectF *rects, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape;
        shape.addRect(rects[index]);
        draw_shape(shape, ShapeKind::Rectangle);
    }
}

void HandPaintEngine::drawLines(const QLineF *lines, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape(lines[index].p1());
        shape.lineTo(lines[index].p2());
        draw_shape(shape, ShapeKind::Open);
    }
}

void HandPaintEngine::drawEllipse(const QRectF &rect) {
    QPainterPath shape;
    shape.addEllipse(rect);
    draw_shape(shape, ShapeKind::Closed);
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
    draw_shape(shape, closed ? ShapeKind::Closed : ShapeKind::Open);
}

// Images reach the target unchanged.
void HandPaintEngine::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) {
    save_target_in_logical_coordinates();
    m_target.drawPixmap(rect, pixmap, source_rect);
    m_target.restore();
}

void HandPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                Qt::ImageConversionFlags flags) {
    save_target_in_logical_coordinates();
    m_target.drawImage(rect, image, source_rect, flags);
    m_target.restore();
}

void HandPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) {
    save_target_in_logical_coordinates();
    m_target.drawTiledPixmap(rect, pixmap, offset);
    m_target.restore();
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

void HandPaintEngine::draw_shape(const QPainterPath &logical_shape, ShapeKind kind) {
    const QPainterPath shape = m_transform.map(logical_shape);
    const RoughOptions options = m_theme.rough_options();
    const std::uint32_t shape_seed = seed(shape);
    const bool filled = m_brush.style() != Qt::NoBrush;
    if (kind != ShapeKind::Open && filled) {
        draw_on_target(m_geometry_engine.fill_area(shape, options, shape_seed), Qt::NoPen, m_brush);
    }
    if (m_pen.style() != Qt::NoPen) {
        // A rectangle with no area is a line: it frames nothing.
        const bool is_frame = kind == ShapeKind::Rectangle && !filled && !shape.boundingRect().isEmpty();
        for (const QPainterPath &outline : m_geometry_engine.outline(shape, options, shape_seed)) {
            draw_on_target(is_frame ? kept_outside(outline, shape) : outline, m_pen, Qt::NoBrush);
        }
    }
}

void HandPaintEngine::draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush) {
    // An outline wobbles up to max_offset away from the shape, and its pen is wide: a frame on
    // the edge of a widget would be cut by the widget's area without this margin.
    const int system_clip_margin =
        pen.style() == Qt::NoPen
            ? 0
            : static_cast<int>(std::ceil(m_theme.rough_options().max_offset + pen.widthF()));
    save_target_in_device_coordinates(system_clip_margin);
    m_target.setPen(pen);
    m_target.setBrush(brush);
    m_target.drawPath(path);
    m_target.restore();
}

void HandPaintEngine::save_target_in_device_coordinates(int system_clip_margin) {
    m_target.save();
    // Intersect keeps the clip the target painter already has.
    // The system clip is the area of the widget Qt is rendering through a shared painter:
    // styles draw past it (for example the fill of a selected tab) and rely on it to cut.
    const QRegion system_clip = systemClip();
    if (!system_clip.isEmpty()) {
        m_target.setClipRegion(grown(system_clip, system_clip_margin), Qt::IntersectClip);
    }
    if (m_clipping) {
        m_target.setClipPath(m_clip, Qt::IntersectClip);
    }
}

void HandPaintEngine::save_target_in_logical_coordinates() {
    save_target_in_device_coordinates();
    // Combine keeps the transform the target painter already has.
    m_target.setTransform(m_transform, true);
}

// `shape` is already in device coordinates: the same primitive drawn at the same place on the
// device gets the same seed, and so the same rough paths.
std::uint32_t HandPaintEngine::seed(const QPainterPath &shape) const { return path_seed(shape); }

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
    save_target_in_logical_coordinates();
    m_target.setPen(m_pen);
    m_target.setFont(font);
    m_target.drawText(position, text_item.text());
    m_target.restore();
}

}  // namespace hpq
