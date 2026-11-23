#include <hpq/RoughGeometryEngine.h>

#include <QLineF>
#include <QPolygonF>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace hpq {

namespace {

// Largest distance, in pixels, between two consecutive points of a stroke.
constexpr double PointSpacing = 4.0;
// Shortest distance, in pixels, between two random wobble offsets along a stroke.
constexpr double MinWobbleSpacing = 30.0;
// The wobble offsets are at least this many amplitudes apart, so that the stroke
// turns gently and never folds back on itself, whatever the amplitude.
constexpr double WobbleSpacingPerAmplitude = 12.0;
// Curves are flattened as if zoomed by this factor, so that they look smooth when drawn.
constexpr double FlatteningZoom = 4.0;
constexpr double Pi = 3.14159265358979323846;
// A shape narrower than this many hachure gaps is too small to hatch: it would get
// too few lines to look filled.
constexpr double MinHachureGapsAcross = 4.0;
// The ends of a hachure move by at most this fraction of the gap.
constexpr double HachureJitterPerGap = 0.25;

// The generator of one shape: the same seeds always give the same numbers.
std::mt19937 shape_generator(std::uint32_t base_seed, std::uint32_t shape_seed)
{
    std::seed_seq seeds{base_seed, shape_seed};
    return std::mt19937(seeds);
}

// A random offset whose length is at most `amplitude`.
QPointF random_offset(std::mt19937 &generator, double amplitude)
{
    std::uniform_real_distribution<double> coordinate(-1.0, 1.0);
    const double x = coordinate(generator);
    const double y = coordinate(generator);
    const QPointF offset = QPointF(x, y) * amplitude;
    const double length = std::hypot(offset.x(), offset.y());
    return length > amplitude ? offset * (amplitude / length) : offset;
}

// An offset that varies smoothly along a path of a given length: random offsets at
// evenly spaced knots, blended with a cosine between two knots.
class SmoothWobble {
public:
    SmoothWobble(double path_length, bool closed, double amplitude, std::mt19937 &generator)
    {
        const double wanted_spacing = std::max(MinWobbleSpacing, WobbleSpacingPerAmplitude * amplitude);
        const int interval_count = std::max(1, static_cast<int>(std::lround(path_length / wanted_spacing)));
        m_knot_spacing = path_length / interval_count;
        for (int i = 0; i <= interval_count; ++i)
            m_knot_offsets.push_back(random_offset(generator, amplitude));
        // A closed path ends where it starts: the same offset there keeps the stroke closed.
        if (closed)
            m_knot_offsets.back() = m_knot_offsets.front();
    }

    QPointF at(double arc_length) const
    {
        if (m_knot_spacing <= 0.0)
            return m_knot_offsets.front();
        const double position = arc_length / m_knot_spacing;
        const int last_interval = static_cast<int>(m_knot_offsets.size()) - 2;
        const int interval = std::clamp(static_cast<int>(std::floor(position)), 0, last_interval);
        const double t = std::clamp(position - interval, 0.0, 1.0);
        const double weight = (1.0 - std::cos(Pi * t)) / 2.0;
        return (1.0 - weight) * m_knot_offsets[interval] + weight * m_knot_offsets[interval + 1];
    }

private:
    std::vector<QPointF> m_knot_offsets;
    double m_knot_spacing = 0.0;
};

double polygon_length(const QPolygonF &polygon)
{
    double length = 0.0;
    for (int i = 1; i < polygon.size(); ++i)
        length += QLineF(polygon[i - 1], polygon[i]).length();
    return length;
}

// The polygon drawn by hand: every point is moved by a smooth wobble, and long
// sides get extra points so that they bend with it.
QPainterPath rough_stroke(const QPolygonF &polygon, double amplitude, std::mt19937 &generator)
{
    const SmoothWobble wobble(polygon_length(polygon), polygon.isClosed(), amplitude, generator);

    QPainterPath stroke(polygon.first() + wobble.at(0.0));
    double arc_length = 0.0;
    for (int i = 1; i < polygon.size(); ++i) {
        const QLineF side(polygon[i - 1], polygon[i]);
        const int point_count = std::max(1, static_cast<int>(std::ceil(side.length() / PointSpacing)));
        for (int j = 1; j <= point_count; ++j) {
            const double t = static_cast<double>(j) / point_count;
            stroke.lineTo(side.pointAt(t) + wobble.at(arc_length + t * side.length()));
        }
        arc_length += side.length();
    }
    return stroke;
}

// The subpaths of `shape` as polygons, with curves cut finely enough to look smooth.
QList<QPolygonF> flattened_subpaths(const QPainterPath &shape)
{
    const QTransform zoom = QTransform::fromScale(FlatteningZoom, FlatteningZoom);
    const QTransform unzoom = zoom.inverted();
    QList<QPolygonF> polygons = shape.toSubpathPolygons(zoom);
    for (QPolygonF &polygon : polygons)
        polygon = unzoom.map(polygon);
    return polygons;
}

// Largest random move of a stroke point, from the roughness and max_offset options.
double wobble_amplitude(const RoughOptions &options)
{
    return std::min(options.roughness * options.max_offset, options.max_offset);
}

// Where a horizontal line crosses a side of a polygon, and whether the side goes down (+1) or up (-1).
struct Crossing {
    double x;
    int direction;
};

// A part of a horizontal line, from `left` to `right`.
struct Span {
    double left;
    double right;
};

// The crossings of the horizontal line at `y` with the sides of `polygons`, from left to right.
// A polygon that is not closed gets a side from its last point to its first, as a fill does.
std::vector<Crossing> crossings_at(const QList<QPolygonF> &polygons, double y)
{
    std::vector<Crossing> crossings;
    for (const QPolygonF &polygon : polygons) {
        for (int i = 0; i < polygon.size(); ++i) {
            const QPointF from = polygon[i];
            const QPointF to = polygon[(i + 1) % polygon.size()];
            // The top end of a side belongs to it, the bottom end does not: a line through
            // a vertex crosses one of the two sides that meet there.
            if ((from.y() <= y) == (to.y() <= y))
                continue;
            const double t = (y - from.y()) / (to.y() - from.y());
            crossings.push_back({from.x() + t * (to.x() - from.x()), to.y() > from.y() ? 1 : -1});
        }
    }
    std::sort(crossings.begin(), crossings.end(),
              [](const Crossing &a, const Crossing &b) { return a.x < b.x; });
    return crossings;
}

// The parts of the horizontal line at `y` that are inside `polygons`, for a fill rule.
std::vector<Span> spans_inside(const QList<QPolygonF> &polygons, double y, Qt::FillRule fill_rule)
{
    const std::vector<Crossing> crossings = crossings_at(polygons, y);
    std::vector<Span> spans;
    int winding = 0;
    for (std::size_t i = 0; i + 1 < crossings.size(); ++i) {
        winding += fill_rule == Qt::WindingFill ? crossings[i].direction : 1;
        const bool inside = fill_rule == Qt::WindingFill ? winding != 0 : winding % 2 != 0;
        if (inside)
            spans.push_back({crossings[i].x, crossings[i + 1].x});
    }
    return spans;
}

}  // namespace

RoughGeometryEngine::RoughGeometryEngine() : RoughGeometryEngine(std::random_device()()) {}

RoughGeometryEngine::RoughGeometryEngine(std::uint32_t base_seed) : m_base_seed(base_seed) {}

std::vector<QPainterPath> RoughGeometryEngine::outline(const QPainterPath &shape, const RoughOptions &options,
                                                       std::uint32_t seed) const
{
    std::mt19937 generator = shape_generator(m_base_seed, seed);
    const double amplitude = wobble_amplitude(options);
    const QList<QPolygonF> polygons = flattened_subpaths(shape);

    std::vector<QPainterPath> strokes;
    for (int stroke_index = 0; stroke_index < options.stroke_count; ++stroke_index) {
        QPainterPath stroke;
        for (const QPolygonF &polygon : polygons) {
            if (!polygon.isEmpty())
                stroke.addPath(rough_stroke(polygon, amplitude, generator));
        }
        strokes.push_back(stroke);
    }
    return strokes;
}

QPainterPath RoughGeometryEngine::fill_area(const QPainterPath &shape, const RoughOptions &options,
                                            std::uint32_t seed) const
{
    std::mt19937 generator = shape_generator(m_base_seed, seed);
    const double amplitude = wobble_amplitude(options);

    QPainterPath area;
    area.setFillRule(shape.fillRule());
    for (const QPolygonF &polygon : flattened_subpaths(shape)) {
        if (polygon.isEmpty())
            continue;
        QPainterPath boundary = rough_stroke(polygon, amplitude, generator);
        boundary.closeSubpath();
        area.addPath(boundary);
    }
    // The area wobbles outward only: it always covers the whole shape, so that adjacent fills
    // (for example the rectangles of a widget background region) leave no unpainted seam.
    return area.united(shape);
}

QPainterPath RoughGeometryEngine::hachure(const QPainterPath &shape, const RoughOptions &options,
                                          std::uint32_t seed) const
{
    QPainterPath lines;
    const double gap = options.hachure_gap;
    const QRectF bounds = shape.boundingRect();
    if (gap <= 0.0 || std::min(bounds.width(), bounds.height()) < MinHachureGapsAcross * gap)
        return lines;

    std::mt19937 generator = shape_generator(m_base_seed, seed);
    const double jitter = std::min(wobble_amplitude(options), HachureJitterPerGap * gap);
    // Turned back by the angle, the hachures are horizontal lines.
    const QTransform to_device = QTransform().rotate(options.hachure_angle);
    const QPainterPath turned_shape = to_device.inverted().map(shape);
    const QList<QPolygonF> polygons = turned_shape.toSubpathPolygons();
    const QRectF extent = turned_shape.boundingRect();

    // The lines are on a grid that starts at the origin of the coordinates, not at the shape.
    for (double row = std::floor(extent.top() / gap);; ++row) {
        const double y = (row + 0.5) * gap;
        if (y >= extent.bottom())
            break;
        for (const Span &span : spans_inside(polygons, y, shape.fillRule())) {
            // A line shorter than its jitter would be a random speck.
            if (span.right - span.left <= jitter)
                continue;
            lines.moveTo(to_device.map(QPointF(span.left, y)) + random_offset(generator, jitter));
            lines.lineTo(to_device.map(QPointF(span.right, y)) + random_offset(generator, jitter));
        }
    }
    return lines;
}

}  // namespace hpq
