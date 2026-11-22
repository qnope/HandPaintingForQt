#include <hpq/RoughGeometryEngine.h>

#include <hpq/Theme.h>

#include <QLineF>
#include <QPointF>

#include <cmath>

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

} // namespace

HandDrawnShape RoughGeometryEngine::hand_draw(const QPainterPath& ideal, const Theme& theme,
                                              RandomGenerator& random) const
{
    QPainterPath strokes;
    QPointF current;
    for (int i = 0; i < ideal.elementCount(); ++i)
    {
        const QPainterPath::Element element = ideal.elementAt(i);
        const QPointF point(element.x, element.y);
        if (element.isLineTo())
            add_rough_segment(strokes, QLineF(current, point), theme, random);
        current = point;
    }
    return HandDrawnShape{strokes, ideal};
}

} // namespace hpq
