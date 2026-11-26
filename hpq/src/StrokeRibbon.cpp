#include "StrokeRibbon.h"

#include "SmoothCurve.h"

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
// The ribbon is at most this much wider across a corner than along the sides.
constexpr double MiterLimit = 1.6;

// A level in [0, 1] that varies smoothly along a stroke of `length`: random levels about
// `wanted_spacing` apart.
SmoothCurve<double> smooth_level(double length, double wanted_spacing, bool closed, std::mt19937 &generator) {
    return SmoothCurve<double>(length, wanted_spacing, closed,
                               [&] { return std::uniform_real_distribution<double>(0.0, 1.0)(generator); });
}

// The width of the ribbon of a piece of chalk along one stroke, as a part of the width of the pen.
class ChalkPressure {
public:
    ChalkPressure(double length, bool closed, std::mt19937 &generator)
        : m_length(length),
          m_closed(closed),
          m_hand(smooth_level(length, PressureSpacing, closed, generator)),
          m_shake(smooth_level(length, ShakeSpacing, closed, generator)),
          m_gap(smooth_level(length, GapSpacing, closed, generator)) {}

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
    // Distance, in pixels, over which the pressure of the hand changes.
    static constexpr double PressureSpacing = 40.0;
    // The pressure makes the ribbon between these parts of its width.
    static constexpr double LightestPressure = 0.65;
    static constexpr double HeaviestPressure = 1.30;
    // Distance, in pixels, over which the chalk shakes on the grain, and how much of the width
    // it adds or removes.
    static constexpr double ShakeSpacing = 5.0;
    static constexpr double ShakeAmount = 0.18;
    // Distance, in pixels, over which the chalk can skip. It skips where a random level in [0, 1]
    // is under GapLevel, and is back to its full width GapEdge above it.
    static constexpr double GapSpacing = 12.0;
    static constexpr double GapLevel = 0.04;
    static constexpr double GapEdge = 0.10;
    // A stroke shorter than this, in pixels, has no gap: a check mark or an arrow stays whole.
    static constexpr double ShortestStrokeWithGaps = 30.0;
    // An open stroke starts this much wider than its width, and ends this much narrower.
    static constexpr double StartEmphasis = 0.12;
    static constexpr double EndFading = 0.20;

    double m_length;
    bool m_closed;
    SmoothCurve<double> m_hand;
    SmoothCurve<double> m_shake;
    SmoothCurve<double> m_gap;
};

// The width of the ribbon of a brush along one stroke, as a part of the width of the brush.
class BrushPressure {
public:
    BrushPressure(double length, bool closed, std::mt19937 &generator)
        : m_length(length), m_closed(closed), m_hand(smooth_level(length, PressureSpacing, closed, generator)) {}

    double at(double arc_length) const {
        const double pressure = LightestPressure + (HeaviestPressure - LightestPressure) * m_hand.at(arc_length);
        if (m_length <= 0.0) {
            return pressure;
        }
        if (m_closed) {
            return pressure *
                   (1.0 + ClosedStartEmphasis - (ClosedStartEmphasis + ClosedEndFading) * arc_length / m_length);
        }
        const double head_length = std::min(HeadLength, HeadPart * m_length);
        const double tail_length = std::min(TailLength, TailPart * m_length);
        const double landing = 1.0 + HeadEmphasis * (1.0 - smooth_step(0.0, head_length, arc_length));
        const double lifting = TipPressure + (1.0 - TipPressure) * smooth_step(0.0, tail_length, m_length - arc_length);
        return pressure * landing * lifting;
    }

private:
    // Distance, in pixels, over which the pressure of the hand changes.
    static constexpr double PressureSpacing = 45.0;
    // The pressure makes the ribbon between these parts of its width.
    static constexpr double LightestPressure = 0.55;
    static constexpr double HeaviestPressure = 1.45;
    // The brush lands at the start of an open stroke: the ribbon is this much wider there, and
    // back to its width HeadLength pixels further, or after HeadPart of a short stroke.
    static constexpr double HeadEmphasis = 0.45;
    static constexpr double HeadLength = 12.0;
    static constexpr double HeadPart = 0.3;
    // The brush lifts at the end of an open stroke: over the last TailLength pixels, or the last
    // TailPart of a short stroke, the ribbon thins to TipPressure of its width.
    static constexpr double TailLength = 40.0;
    static constexpr double TailPart = 0.5;
    static constexpr double TipPressure = 0.12;
    // Around a closed stroke the brush runs out of ink: it starts this much wider than its
    // width, and ends this much narrower.
    static constexpr double ClosedStartEmphasis = 0.25;
    static constexpr double ClosedEndFading = 0.40;

    double m_length;
    bool m_closed;
    SmoothCurve<double> m_hand;
};

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

// The ribbon of one polygon of a stroke: its left edge, then its right edge backwards. It is
// `width` pixels wide where a `Pressure` of the hand is 1.
template <typename Pressure>
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
        // The tip of the tool is as wide as its mark: an open stroke starts and ends half a
        // width further.
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

// The area a tool marks when it follows `stroke`: `width` pixels wide where its `Pressure` is 1,
// wider and narrower as the pressure changes along each stroke. The random numbers of the
// pressure come from `seed`.
template <typename Pressure>
QPainterPath stroke_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    // The generator of one stroke: the same seed always gives the same numbers.
    std::seed_seq seeds{seed};
    std::mt19937 generator(seeds);
    QPainterPath ribbon;
    // The edges of a closed stroke are two loops around the same point: the winding rule
    // fills what is between them, and a ribbon that overlaps itself in a sharp corner.
    ribbon.setFillRule(Qt::WindingFill);
    for (const QPolygonF &polygon : stroke.toSubpathPolygons()) {
        const QPolygonF area = ribbon_polygon<Pressure>(polygon, width, generator);
        if (!area.isEmpty()) {
            ribbon.addPolygon(area);
            ribbon.closeSubpath();
        }
    }
    return ribbon;
}

}  // namespace

QPainterPath chalk_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    return stroke_ribbon<ChalkPressure>(stroke, width, seed);
}

QPainterPath brush_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    return stroke_ribbon<BrushPressure>(stroke, width, seed);
}

}  // namespace hpq
