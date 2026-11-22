#include <hpq/RoughGeometryEngine.h>

#include "SeededRandom.h"

#include <QLineF>
#include <QPolygonF>
#include <QTransform>

#include <algorithm>
#include <cmath>
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

// A random offset whose length is at most `amplitude`.
QPointF random_offset(SeededRandom &random, double amplitude)
{
    const QPointF offset = QPointF(random.next_signed(), random.next_signed()) * amplitude;
    const double length = std::hypot(offset.x(), offset.y());
    return length > amplitude ? offset * (amplitude / length) : offset;
}

// An offset that varies smoothly along a path of a given length: random offsets at
// evenly spaced knots, blended with a cosine between two knots.
class SmoothWobble {
public:
    SmoothWobble(double path_length, bool closed, double amplitude, SeededRandom &random)
    {
        const double wanted_spacing = std::max(MinWobbleSpacing, WobbleSpacingPerAmplitude * amplitude);
        const int interval_count = std::max(1, static_cast<int>(std::lround(path_length / wanted_spacing)));
        m_knot_spacing = path_length / interval_count;
        for (int i = 0; i <= interval_count; ++i)
            m_knot_offsets.push_back(random_offset(random, amplitude));
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
QPainterPath rough_stroke(const QPolygonF &polygon, double amplitude, SeededRandom &random)
{
    const SmoothWobble wobble(polygon_length(polygon), polygon.isClosed(), amplitude, random);

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

}  // namespace

std::vector<QPainterPath> RoughGeometryEngine::outline(const QPainterPath &shape, const RoughOptions &options,
                                                       std::uint32_t seed) const
{
    SeededRandom random(seed);
    const double amplitude = wobble_amplitude(options);
    const QList<QPolygonF> polygons = flattened_subpaths(shape);

    std::vector<QPainterPath> strokes;
    for (int stroke_index = 0; stroke_index < options.stroke_count; ++stroke_index) {
        QPainterPath stroke;
        for (const QPolygonF &polygon : polygons) {
            if (!polygon.isEmpty())
                stroke.addPath(rough_stroke(polygon, amplitude, random));
        }
        strokes.push_back(stroke);
    }
    return strokes;
}

QPainterPath RoughGeometryEngine::fill_area(const QPainterPath &shape, const RoughOptions &options,
                                            std::uint32_t seed) const
{
    SeededRandom random(seed);
    const double amplitude = wobble_amplitude(options);

    QPainterPath area;
    area.setFillRule(shape.fillRule());
    for (const QPolygonF &polygon : flattened_subpaths(shape)) {
        if (polygon.isEmpty())
            continue;
        QPainterPath boundary = rough_stroke(polygon, amplitude, random);
        boundary.closeSubpath();
        area.addPath(boundary);
    }
    // The area wobbles outward only: it always covers the whole shape, so that adjacent fills
    // (for example the rectangles of a widget background region) leave no unpainted seam.
    return area.united(shape);
}

}  // namespace hpq
