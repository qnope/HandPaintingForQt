#include "ChalkStroke.h"

#include <QLineF>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace hpq {

namespace {

// Largest distance, in pixels, between two consecutive points of a ribbon.
constexpr double PointSpacing = 3.0;
// Distance, in pixels, over which the pressure of the hand changes.
constexpr double PressureSpacing = 40.0;
// The pressure makes the ribbon between these parts of its width.
constexpr double LightestPressure = 0.65;
constexpr double HeaviestPressure = 1.30;
// Distance, in pixels, over which the chalk shakes on the grain, and how much of the width
// it adds or removes.
constexpr double ShakeSpacing = 5.0;
constexpr double ShakeAmount = 0.18;
// Distance, in pixels, over which the chalk can skip. It skips where a random level in [0, 1]
// is under GapLevel, and is back to its full width GapEdge above it.
constexpr double GapSpacing = 12.0;
constexpr double GapLevel = 0.04;
constexpr double GapEdge = 0.10;
// A stroke shorter than this, in pixels, has no gap: a check mark or an arrow stays whole.
constexpr double ShortestStrokeWithGaps = 30.0;
// An open stroke starts this much wider than its width, and ends this much narrower.
constexpr double StartEmphasis = 0.12;
constexpr double EndFading = 0.20;
// The ribbon is at most this much wider across a corner than along the sides.
constexpr double MiterLimit = 1.6;
constexpr double Pi = 3.14159265358979323846;

// A random level in [0, 1).
double random_level(std::mt19937 &generator) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(generator);
}

// A level in [0, 1] that varies smoothly along a path of a given length: random levels at
// evenly spaced knots, blended with a cosine between two knots.
class SmoothLevel {
public:
    SmoothLevel(double path_length, double wanted_spacing, bool closed, std::mt19937 &generator) {
        const int interval_count = std::max(1, static_cast<int>(std::lround(path_length / wanted_spacing)));
        m_knot_spacing = path_length / interval_count;
        for (int index = 0; index <= interval_count; ++index) {
            m_knot_levels.push_back(random_level(generator));
        }
        // A closed path ends where it starts: the same level there leaves no step in the ribbon.
        if (closed) {
            m_knot_levels.back() = m_knot_levels.front();
        }
    }

    double at(double arc_length) const {
        if (m_knot_spacing <= 0.0) {
            return m_knot_levels.front();
        }
        const double position = arc_length / m_knot_spacing;
        const int last_interval = static_cast<int>(m_knot_levels.size()) - 2;
        const int interval = std::clamp(static_cast<int>(std::floor(position)), 0, last_interval);
        const double t = std::clamp(position - interval, 0.0, 1.0);
        const double weight = (1.0 - std::cos(Pi * t)) / 2.0;
        return (1.0 - weight) * m_knot_levels[interval] + weight * m_knot_levels[interval + 1];
    }

private:
    std::vector<double> m_knot_levels;
    double m_knot_spacing = 0.0;
};

double smooth_step(double from, double to, double value) {
    const double t = std::clamp((value - from) / (to - from), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

QPointF normalized(const QPointF &vector) {
    const double length = std::hypot(vector.x(), vector.y());
    return length > 0.0 ? vector / length : QPointF();
}

// The points of `polygon` without consecutive duplicates, with extra points on its long sides
// so that the width can change along them.
std::vector<QPointF> spaced_points(const QPolygonF &polygon) {
    std::vector<QPointF> points{polygon.first()};
    for (int index = 1; index < polygon.size(); ++index) {
        const QLineF side(points.back(), polygon[index]);
        if (side.length() <= 0.0) {
            continue;
        }
        const int point_count = static_cast<int>(std::ceil(side.length() / PointSpacing));
        for (int step = 1; step <= point_count; ++step) {
            points.push_back(side.pointAt(static_cast<double>(step) / point_count));
        }
    }
    return points;
}

// The width of the ribbon along one stroke, as a part of the width of the pen.
class Pressure {
public:
    Pressure(double length, bool closed, std::mt19937 &generator)
        : m_length(length),
          m_closed(closed),
          m_hand(length, PressureSpacing, closed, generator),
          m_shake(length, ShakeSpacing, closed, generator),
          m_gap(length, GapSpacing, closed, generator) {}

    double at(double arc_length) const {
        double pressure = LightestPressure + (HeaviestPressure - LightestPressure) * m_hand.at(arc_length);
        pressure *= 1.0 + ShakeAmount * (2.0 * m_shake.at(arc_length) - 1.0);
        if (!m_closed && m_length > 0.0) {
            pressure *= 1.0 + StartEmphasis - (StartEmphasis + EndFading) * arc_length / m_length;
        }
        if (m_length >= ShortestStrokeWithGaps) {
            pressure *= smooth_step(GapLevel, GapLevel + GapEdge, m_gap.at(arc_length));
        }
        return pressure;
    }

private:
    double m_length;
    bool m_closed;
    SmoothLevel m_hand;
    SmoothLevel m_shake;
    SmoothLevel m_gap;
};

// The ribbon of one polygon of a stroke: its left edge, then its right edge backwards.
QPolygonF ribbon_polygon(const QPolygonF &polygon, double width, std::mt19937 &generator) {
    const std::vector<QPointF> points = spaced_points(polygon);
    const int count = static_cast<int>(points.size());
    if (count < 2) {
        return QPolygonF();
    }
    const bool closed = points.front() == points.back();

    std::vector<double> arc_lengths(points.size(), 0.0);
    for (int index = 1; index < count; ++index) {
        arc_lengths[index] = arc_lengths[index - 1] + QLineF(points[index - 1], points[index]).length();
    }
    const Pressure pressure(arc_lengths.back(), closed, generator);

    // The left edge, followed at the end by the right edge backwards.
    QPolygonF edges;
    std::vector<QPointF> right;
    for (int index = 0; index < count; ++index) {
        // The points before and after this one; on a closed stroke the last point is the first.
        const bool has_previous = closed || index > 0;
        const bool has_next = closed || index < count - 1;
        const QPointF &previous = points[index > 0 ? index - 1 : count - 2];
        const QPointF &next = points[index < count - 1 ? index + 1 : 1];
        const QPointF incoming = has_previous ? normalized(points[index] - previous) : QPointF();
        const QPointF outgoing = has_next ? normalized(next - points[index]) : QPointF();
        const QPointF tangent = normalized(incoming + outgoing);
        const QPointF normal(-tangent.y(), tangent.x());
        // Across a corner the ribbon is cut along the bisector: widen it to keep its width.
        const double half_turn_cosine =
            has_previous && has_next ? std::sqrt(std::max(0.0, (1.0 + QPointF::dotProduct(incoming, outgoing)) / 2.0))
                                     : 1.0;
        const double miter = std::min(MiterLimit, 1.0 / std::max(half_turn_cosine, 1.0 / MiterLimit));
        const double half_width = width * pressure.at(arc_lengths[index]) * miter / 2.0;
        // The chalk has a blunt tip: an open stroke starts and ends half a width further.
        QPointF center = points[index];
        if (!has_previous) {
            center -= tangent * half_width;
        } else if (!has_next) {
            center += tangent * half_width;
        }
        edges.append(center + normal * half_width);
        right.push_back(center - normal * half_width);
    }
    for (auto point = right.rbegin(); point != right.rend(); ++point) {
        edges.append(*point);
    }
    return edges;
}

}  // namespace

QPainterPath chalk_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    // The generator of one stroke: the same seed always gives the same numbers.
    std::seed_seq seeds{seed};
    std::mt19937 generator(seeds);
    QPainterPath ribbon;
    // The edges of a closed stroke are two loops around the same point: the winding rule
    // fills what is between them, and a ribbon that overlaps itself in a sharp corner.
    ribbon.setFillRule(Qt::WindingFill);
    for (const QPolygonF &polygon : stroke.toSubpathPolygons()) {
        const QPolygonF area = ribbon_polygon(polygon, width, generator);
        if (!area.isEmpty()) {
            ribbon.addPolygon(area);
            ribbon.closeSubpath();
        }
    }
    return ribbon;
}

}  // namespace hpq
