#include "BrushStroke.h"

#include "StrokeRibbon.h"

#include <algorithm>

namespace hpq {

namespace {

// Distance, in pixels, over which the pressure of the hand changes.
constexpr double PressureSpacing = 45.0;
// The pressure makes the ribbon between these parts of its width.
constexpr double LightestPressure = 0.55;
constexpr double HeaviestPressure = 1.45;
// The brush lands at the start of an open stroke: the ribbon is this much wider there, and
// back to its width HeadLength pixels further, or after HeadPart of a short stroke.
constexpr double HeadEmphasis = 0.45;
constexpr double HeadLength = 12.0;
constexpr double HeadPart = 0.3;
// The brush lifts at the end of an open stroke: over the last TailLength pixels, or the last
// TailPart of a short stroke, the ribbon thins to TipPressure of its width.
constexpr double TailLength = 40.0;
constexpr double TailPart = 0.5;
constexpr double TipPressure = 0.12;
// Around a closed stroke the brush runs out of ink: it starts this much wider than its
// width, and ends this much narrower.
constexpr double ClosedStartEmphasis = 0.25;
constexpr double ClosedEndFading = 0.40;

// The width of the ribbon along one stroke, as a part of the width of the brush.
class Pressure {
public:
    Pressure(double length, bool closed, std::mt19937 &generator)
        : m_length(length), m_closed(closed), m_hand(length, PressureSpacing, closed, generator) {}

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
    double m_length;
    bool m_closed;
    SmoothLevel m_hand;
};

}  // namespace

QPainterPath brush_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed) {
    return stroke_ribbon(stroke, width, seed, [](double length, bool closed, std::mt19937 &generator) {
        return [pressure = Pressure(length, closed, generator)](double arc_length) { return pressure.at(arc_length); };
    });
}

}  // namespace hpq
