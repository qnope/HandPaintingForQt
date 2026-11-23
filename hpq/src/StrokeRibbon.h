#pragma once

#include <QPainterPath>

#include <cstdint>
#include <functional>
#include <random>
#include <vector>

namespace hpq {

// A level in [0, 1] that varies smoothly along a path of a given length: random levels at
// evenly spaced knots, blended with a cosine between two knots.
class SmoothLevel {
public:
    SmoothLevel(double path_length, double wanted_spacing, bool closed, std::mt19937 &generator);

    double at(double arc_length) const;

private:
    std::vector<double> m_knot_levels;
    double m_knot_spacing = 0.0;
};

// 0 at `from` and below, 1 at `to` and above, and a smooth step between them.
double smooth_step(double from, double to, double value);

// The width of a ribbon at a distance along its stroke, as a part of the width of the tool.
using RibbonPressure = std::function<double(double arc_length)>;

// Makes the pressure of the hand along one stroke of a given length, open or closed. The
// random numbers it needs come from the generator of the ribbon.
using RibbonHand = std::function<RibbonPressure(double length, bool closed, std::mt19937 &generator)>;

// The area a tool marks when it follows `stroke`, to be filled instead of stroking it.
// It is `width` pixels wide where the pressure of `hand` is 1, wider and narrower as the
// pressure changes along each stroke.
// The result depends only on the stroke, the width, the seed and the hand.
QPainterPath stroke_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed, const RibbonHand &hand);

}  // namespace hpq
