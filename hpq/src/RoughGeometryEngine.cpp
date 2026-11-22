#include <hpq/RoughGeometryEngine.h>

#include "SeededRandom.h"

#include <QLineF>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace hpq {

namespace {

// Distance, in pixels, between two consecutive points of a rough segment.
constexpr double PointSpacing = 10.0;
constexpr double Pi = 3.14159265358979323846;

QPointF clamp_length(QPointF vector, double max_length)
{
    const double length = std::hypot(vector.x(), vector.y());
    return length > max_length ? vector * (max_length / length) : vector;
}

// A random offset whose length is at most `amplitude`.
QPointF random_offset(SeededRandom &random, double amplitude)
{
    const QPointF direction(random.next_signed(), random.next_signed());
    return clamp_length(direction * amplitude, amplitude);
}

// Adds to `stroke` the points of a wobbly line from `from` to `to`. The ends are
// moved by `from_offset` and `to_offset`, the middle bows sideways; no point is
// farther than `max_offset` from the segment.
void add_rough_segment(QPainterPath &stroke, QPointF from, QPointF to, QPointF from_offset,
                       QPointF to_offset, double bow, double max_offset)
{
    const QLineF segment(from, to);
    const QLineF unit_normal = segment.normalVector().unitVector();
    const QPointF normal = unit_normal.p2() - unit_normal.p1();
    const int point_count = std::max(2, static_cast<int>(std::ceil(segment.length() / PointSpacing)));
    for (int i = 1; i <= point_count; ++i) {
        const double t = static_cast<double>(i) / point_count;
        const QPointF offset = (1.0 - t) * from_offset + t * to_offset + std::sin(Pi * t) * bow * normal;
        stroke.lineTo(segment.pointAt(t) + clamp_length(offset, max_offset));
    }
}

QPainterPath rough_stroke(const QPolygonF &polygon, double amplitude, double max_offset,
                          SeededRandom &random)
{
    std::vector<QPointF> vertex_offsets;
    vertex_offsets.reserve(polygon.size());
    for (int i = 0; i < polygon.size(); ++i)
        vertex_offsets.push_back(random_offset(random, amplitude));

    QPainterPath stroke(polygon.first() + vertex_offsets.front());
    for (int i = 1; i < polygon.size(); ++i) {
        const double bow = random.next_signed() * amplitude;
        add_rough_segment(stroke, polygon[i - 1], polygon[i], vertex_offsets[i - 1], vertex_offsets[i], bow,
                          max_offset);
    }
    return stroke;
}

}  // namespace

std::vector<QPainterPath> RoughGeometryEngine::outline(const QPainterPath &shape, const RoughOptions &options,
                                                       std::uint32_t seed) const
{
    SeededRandom random(seed);
    const double amplitude = std::min(options.roughness * options.max_offset, options.max_offset);
    const QList<QPolygonF> polygons = shape.toSubpathPolygons();

    std::vector<QPainterPath> strokes;
    for (int stroke_index = 0; stroke_index < options.stroke_count; ++stroke_index) {
        QPainterPath stroke;
        for (const QPolygonF &polygon : polygons) {
            if (!polygon.isEmpty())
                stroke.addPath(rough_stroke(polygon, amplitude, options.max_offset, random));
        }
        strokes.push_back(stroke);
    }
    return strokes;
}

}  // namespace hpq
