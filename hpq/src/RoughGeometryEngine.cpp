#include <hpq/RoughGeometryEngine.h>

#include <QLineF>
#include <QPolygonF>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
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

// True when `polygon` is a rectangle with its sides along the axes: it goes once around, its
// four sides in turn along x and along y.
bool is_upright_rectangle(const QPolygonF &polygon)
{
    if (polygon.size() != 5 || !polygon.isClosed())
        return false;
    const bool starts_along_x = polygon[0].y() == polygon[1].y();
    for (int i = 0; i < 4; ++i) {
        const QPointF side = polygon[i + 1] - polygon[i];
        const bool along_x = (i % 2 == 0) == starts_along_x;
        const double along = along_x ? side.x() : side.y();
        const double across = along_x ? side.y() : side.x();
        if (along == 0.0 || across != 0.0)
            return false;
    }
    return true;
}

double cross(const QPointF &a, const QPointF &b)
{
    return a.x() * b.y() - a.y() * b.x();
}

// Where a segment goes through a convex polygon: the fractions of the segment where it goes in
// and out, and the sides it crosses there; no side where it starts or ends inside.
struct Passage {
    double in = 0.0;
    double out = 1.0;
    int in_side = -1;
    int out_side = -1;
};

// How the segment from `from` to `to` goes through the closed polygon `convex`, whose corners
// all turn the way of `turn` (1 or -1); none when it misses the polygon.
std::optional<Passage> passage_through(const QPointF &from, const QPointF &to, const QPolygonF &convex, int turn)
{
    Passage passage;
    const QPointF direction = to - from;
    for (int side = 0; side + 1 < convex.size(); ++side) {
        const QPointF along = convex[side + 1] - convex[side];
        // How far outside the side the segment starts, and how fast it goes out of it.
        const double start = -turn * cross(along, from - convex[side]);
        const double speed = -turn * cross(along, direction);
        if (speed == 0.0) {
            if (start > 0.0)
                return std::nullopt;
            continue;
        }
        const double t = -start / speed;
        if (speed < 0.0 && t >= passage.in) {
            passage.in = t;
            passage.in_side = side;
        } else if (speed > 0.0 && t < passage.out) {
            passage.out = t;
            passage.out_side = side;
        }
    }
    if (passage.in > passage.out)
        return std::nullopt;
    return passage;
}

// The outline of what `boundary` and `rectangle` cover together, when `boundary` goes around
// the rectangle the same way and never crosses itself: `boundary` where it is outside the
// rectangle, and the sides of the rectangle where it goes inside. None when rounding leaves
// a doubt, for example when the boundary goes exactly along a side.
std::optional<QPolygonF> united_with_rectangle(const QPolygonF &boundary, const QPolygonF &rectangle)
{
    const int count = boundary.isClosed() ? boundary.size() - 1 : boundary.size();
    const QRectF bounds = rectangle.boundingRect();
    // The outline starts from a point outside the rectangle; with none, the boundary is inside.
    int start = 0;
    while (start < count && bounds.contains(boundary[start]))
        ++start;
    if (start == count)
        return rectangle;
    const int turn = cross(rectangle[1] - rectangle[0], rectangle[2] - rectangle[1]) > 0.0 ? 1 : -1;

    QPolygonF outline{boundary[start]};
    // Where the boundary went into the rectangle, and through which side, while it is inside.
    QPointF entry;
    int entry_side = -1;
    // Goes on along the sides of the rectangle, by its corners, from the entry to the exit:
    // once around when the boundary comes out of the side it went in by, behind where it went in.
    const auto follow_sides = [&](int exit_side, const QPointF &exit) {
        const QPointF along = rectangle[entry_side + 1] - rectangle[entry_side];
        int side = entry_side;
        if (exit_side == entry_side && QPointF::dotProduct(exit - entry, along) < 0.0) {
            outline << rectangle[side + 1];
            side = (side + 1) % 4;
        }
        for (; side != exit_side; side = (side + 1) % 4)
            outline << rectangle[side + 1];
    };
    for (int i = start; i < start + count; ++i) {
        const QPointF from = boundary[i % count];
        const QPointF to = boundary[(i + 1) % count];
        const std::optional<Passage> passage = passage_through(from, to, rectangle, turn);
        const auto point_at = [&](double t) { return from + t * (to - from); };
        if (entry_side < 0) {
            // A segment that misses the rectangle, or only touches it, stays in the outline.
            if (!passage || passage->in == passage->out) {
                outline << to;
                continue;
            }
            if (passage->in_side < 0)
                return std::nullopt;
            entry = point_at(passage->in);
            entry_side = passage->in_side;
            outline << entry;
            if (passage->out == 1.0)
                continue;
        } else if (!passage || passage->in > 0.0) {
            return std::nullopt;
        } else if (passage->out == 1.0) {
            continue;
        }
        const QPointF exit = point_at(passage->out);
        follow_sides(passage->out_side, exit);
        entry_side = -1;
        outline << exit << to;
    }
    // QPainterPath::united starts where the boundary first comes out of the rectangle: so does
    // this outline. A stroke of chalk or of a brush along it then gets the same pressure.
    if (start > 0) {
        outline.prepend(outline[outline.size() - 2]);
        outline.removeLast();
    }
    return outline;
}

// Adds a line to `point` as QPainterPath::united does: when the last point is in line with
// the one before it and `point`, `point` takes its place. A stroke of chalk or of a brush along
// the path then gets the same pressure as along the one that united makes.
void add_line_to(QPainterPath &path, const QPointF &point)
{
    const int count = path.elementCount();
    if (count >= 2 && path.elementAt(count - 1).isLineTo()) {
        const QPointF first = path.elementAt(count - 2);
        const QPointF last = path.elementAt(count - 1);
        const QPointF to_point = point - first;
        const QPointF to_last = last - first;
        if (qFuzzyIsNull(QPointF::dotProduct(QPointF(-to_point.y(), to_point.x()), to_last))) {
            path.setElementPositionAt(count - 1, point.x(), point.y());
            return;
        }
    }
    path.lineTo(point);
}

// Largest random move of a stroke point, from the roughness and max_offset options.
double wobble_amplitude(const RoughOptions &options)
{
    return std::min(options.roughness * options.max_offset, options.max_offset);
}

// What `boundary`, a rough stroke around `shape` whose subpaths are `polygons`, covers with
// `shape`. Uniting two paths is slow. The rough boundary of a rectangle higher and wider than
// twice the wobble never crosses itself and goes around the rectangle the same way: a
// rectangle, the most common shape, is united with it by a faster way of its own.
QPainterPath united_with_shape(const QPainterPath &boundary, const QPainterPath &shape,
                               const QList<QPolygonF> &polygons, double amplitude)
{
    const QRectF bounds = shape.boundingRect();
    if (polygons.size() == 1 && is_upright_rectangle(polygons.front()) &&
        std::min(bounds.width(), bounds.height()) > 2.0 * amplitude) {
        // A rough stroke ends where it starts, give or take a rounding.
        QPainterPath closed = boundary;
        closed.closeSubpath();
        if (const std::optional<QPolygonF> outline =
                united_with_rectangle(closed.toSubpathPolygons().front(), polygons.front())) {
            QPainterPath united(outline->front());
            for (int i = 1; i < outline->size(); ++i)
                add_line_to(united, (*outline)[i]);
            return united;
        }
    }
    return boundary.united(shape);
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

std::vector<QPainterPath> RoughGeometryEngine::frame_outline(const QPainterPath &frame, const RoughOptions &options,
                                                             std::uint32_t seed) const
{
    // The outline of this engine, even when a derived engine draws another one: the way of
    // its own to unite a rectangle relies on how this one wobbles.
    std::vector<QPainterPath> strokes = RoughGeometryEngine::outline(frame, options, seed);
    const QList<QPolygonF> polygons = flattened_subpaths(frame);
    const double amplitude = wobble_amplitude(options);
    for (QPainterPath &stroke : strokes) {
        stroke.setFillRule(frame.fillRule());
        stroke = united_with_shape(stroke, frame, polygons, amplitude);
    }
    return strokes;
}

QPainterPath RoughGeometryEngine::fill_area(const QPainterPath &shape, const RoughOptions &options,
                                            std::uint32_t seed) const
{
    std::mt19937 generator = shape_generator(m_base_seed, seed);
    const double amplitude = wobble_amplitude(options);

    const QList<QPolygonF> polygons = flattened_subpaths(shape);
    QPainterPath area;
    area.setFillRule(shape.fillRule());
    for (const QPolygonF &polygon : polygons) {
        if (polygon.isEmpty())
            continue;
        QPainterPath boundary = rough_stroke(polygon, amplitude, generator);
        boundary.closeSubpath();
        area.addPath(boundary);
    }
    // The area wobbles outward only: it always covers the whole shape, so that adjacent fills
    // (for example the rectangles of a widget background region) leave no unpainted seam.
    return united_with_shape(area, shape, polygons, amplitude);
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
