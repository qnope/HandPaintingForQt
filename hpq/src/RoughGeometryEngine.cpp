#include <hpq/RoughGeometryEngine.h>

#include <hpq/Theme.h>

#include <QLineF>
#include <QPointF>

#include <algorithm>
#include <cmath>
#include <vector>

namespace hpq {

namespace {

// A uniform random value in [-bound, bound].
double random_in(double bound, RandomGenerator& random)
{
    return std::uniform_real_distribution<double>(-bound, bound)(random);
}

// A random point within radius of center.
QPointF jittered(const QPointF& center, double radius, RandomGenerator& random)
{
    constexpr double Pi = 3.14159265358979323846;
    const double angle = std::uniform_real_distribution<double>(0.0, 2.0 * Pi)(random);
    const double distance = radius * std::sqrt(std::uniform_real_distribution<double>(0.0, 1.0)(random));
    return center + distance * QPointF(std::cos(angle), std::sin(angle));
}

// One stroke of the segment: its ends move by at most roughness / 2, and it bows from its chord by at most
// roughness / 2 (a quadratic curve deviates from its chord by half the offset of its control point), so every
// point of the stroke stays within roughness of the matching point of the segment.
void add_rough_stroke(QPainterPath& strokes, const QLineF& segment, double roughness, RandomGenerator& random)
{
    const double half_roughness = roughness / 2.0;
    const QPointF start = jittered(segment.p1(), half_roughness, random);
    const QPointF end = jittered(segment.p2(), half_roughness, random);
    const QPointF direction = (segment.p2() - segment.p1()) / segment.length();
    const QPointF normal(-direction.y(), direction.x());
    const QPointF control = (start + end) / 2.0 + random_in(roughness, random) * normal;
    strokes.moveTo(start);
    strokes.quadTo(control, end);
}

void add_rough_segment(QPainterPath& strokes, const QLineF& segment, const Theme& theme, RandomGenerator& random)
{
    if (segment.length() == 0.0)
        return;
    for (int pass = 0; pass < theme.passes(); ++pass)
        add_rough_stroke(strokes, segment, theme.roughness(), random);
}

// The largest distance between two consecutive points of a flattened curve, in pixels.
constexpr double FlatteningStep = 4.0;
// The distance along a curve between two random offsets of a hand-drawn pass, in pixels.
constexpr double WobbleLength = 24.0;

QPointF cubic_point(const QPointF& start, const QPointF& control1, const QPointF& control2, const QPointF& end,
                    double t)
{
    const double u = 1.0 - t;
    return u * u * u * start + 3.0 * u * u * t * control1 + 3.0 * u * t * t * control2 + t * t * t * end;
}

// Appends the points of the cubic curve, start excluded, to points. The control polygon is longer than the
// curve, so consecutive points are at most FlatteningStep apart.
void flatten_cubic(std::vector<QPointF>& points, const QPointF& start, const QPointF& control1,
                   const QPointF& control2, const QPointF& end)
{
    const double control_length =
        QLineF(start, control1).length() + QLineF(control1, control2).length() + QLineF(control2, end).length();
    const int steps = std::max(1, static_cast<int>(std::ceil(control_length / FlatteningStep)));
    for (int step = 1; step <= steps; ++step)
        points.push_back(cubic_point(start, control1, control2, end, static_cast<double>(step) / steps));
}

// The unit normal of the polyline at its point index (zero where the polyline does not move).
QPointF normal_at(const std::vector<QPointF>& points, std::size_t index)
{
    const QPointF direction = points[std::min(index + 1, points.size() - 1)] - points[index > 0 ? index - 1 : 0];
    const double length = std::hypot(direction.x(), direction.y());
    if (length == 0.0)
        return QPointF();
    return QPointF(-direction.y(), direction.x()) / length;
}

// The distance along the polyline from its first point to each of its points.
std::vector<double> distances_along(const std::vector<QPointF>& points)
{
    std::vector<double> distances(points.size(), 0.0);
    for (std::size_t i = 1; i < points.size(); ++i)
        distances[i] = distances[i - 1] + QLineF(points[i - 1], points[i]).length();
    return distances;
}

// A smooth random offset along a curve of the given length: random values in [-bound, bound] every
// WobbleLength or so, eased into each other, so the offset never exceeds bound.
class Wobble
{
public:
    Wobble(double length, double bound, RandomGenerator& random)
        : m_length(length)
    {
        const int intervals = std::max(1, static_cast<int>(std::ceil(length / WobbleLength)));
        for (int i = 0; i <= intervals; ++i)
            m_offsets.push_back(random_in(bound, random));
    }

    double at(double distance) const
    {
        const double position = distance / m_length * (m_offsets.size() - 1);
        const std::size_t index = std::min(static_cast<std::size_t>(position), m_offsets.size() - 2);
        const double fraction = position - index;
        const double eased = fraction * fraction * (3.0 - 2.0 * fraction);
        return m_offsets[index] + eased * (m_offsets[index + 1] - m_offsets[index]);
    }

private:
    double m_length;
    std::vector<double> m_offsets;
};

// Draws a smooth curve through the points (a Catmull-Rom spline made of cubic curves).
void add_smooth_curve(QPainterPath& strokes, const std::vector<QPointF>& points)
{
    const std::size_t last = points.size() - 1;
    strokes.moveTo(points.front());
    for (std::size_t i = 0; i < last; ++i)
    {
        const QPointF& before = points[i > 0 ? i - 1 : 0];
        const QPointF& after = points[std::min(i + 2, last)];
        strokes.cubicTo(points[i] + (points[i + 1] - before) / 6.0, points[i + 1] - (after - points[i]) / 6.0,
                        points[i + 1]);
    }
}

// One stroke of the flattened curve: each point moves along the normal by a smooth random offset of at most
// roughness / 2, then a smooth curve goes through the moved points.
void add_rough_curve_stroke(QPainterPath& strokes, const std::vector<QPointF>& curve,
                            const std::vector<double>& distances, double roughness, RandomGenerator& random)
{
    const Wobble wobble(distances.back(), roughness / 2.0, random);
    std::vector<QPointF> moved;
    moved.reserve(curve.size());
    for (std::size_t i = 0; i < curve.size(); ++i)
        moved.push_back(curve[i] + wobble.at(distances[i]) * normal_at(curve, i));
    add_smooth_curve(strokes, moved);
}

// curve: a flattened run of consecutive curves.
void add_rough_curve(QPainterPath& strokes, const std::vector<QPointF>& curve, const Theme& theme,
                     RandomGenerator& random)
{
    if (curve.size() < 2)
        return;
    const std::vector<double> distances = distances_along(curve);
    if (distances.back() == 0.0)
        return;
    for (int pass = 0; pass < theme.passes(); ++pass)
        add_rough_curve_stroke(strokes, curve, distances, theme.roughness(), random);
}

} // namespace

HandDrawnShape RoughGeometryEngine::hand_draw(const QPainterPath& ideal, const Theme& theme,
                                              RandomGenerator& random) const
{
    QPainterPath strokes;
    QPointF current;
    std::vector<QPointF> curve; // the flattened run of consecutive curves being read
    for (int i = 0; i < ideal.elementCount(); ++i)
    {
        const QPainterPath::Element element = ideal.elementAt(i);
        const QPointF point(element.x, element.y);
        if (element.isCurveTo())
        {
            // A curve element is followed by its second control point and its end point.
            const QPointF control2 = ideal.elementAt(i + 1);
            const QPointF end = ideal.elementAt(i + 2);
            if (curve.empty())
                curve.push_back(current);
            flatten_cubic(curve, current, point, control2, end);
            current = end;
            i += 2;
            continue;
        }
        add_rough_curve(strokes, curve, theme, random);
        curve.clear();
        if (element.isLineTo())
            add_rough_segment(strokes, QLineF(current, point), theme, random);
        current = point;
    }
    add_rough_curve(strokes, curve, theme, random);
    return HandDrawnShape{strokes, ideal};
}

} // namespace hpq
