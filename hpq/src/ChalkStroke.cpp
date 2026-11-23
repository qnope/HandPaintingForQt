#include "ChalkStroke.h"

#include "StrokeRibbon.h"

namespace hpq {

namespace {

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

}  // namespace

QPainterPath chalk_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    return stroke_ribbon(stroke, width, seed, [](double length, bool closed, std::mt19937 &generator) {
        return [pressure = Pressure(length, closed, generator)](double arc_length) { return pressure.at(arc_length); };
    });
}

}  // namespace hpq
