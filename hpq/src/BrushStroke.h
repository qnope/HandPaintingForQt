#pragma once

#include <QPainterPath>

#include <cstdint>

namespace hpq {

// The area a brush loaded with ink marks when it follows `stroke`, to be filled instead of
// stroking it. It is `width` pixels wide on average: the pressure of the hand makes it swell
// and thin along the stroke. The brush lands heavily where an open stroke starts and lifts
// to a point where it ends; around a closed stroke it runs out of ink, and comes back
// thinner to where it started.
// The result depends only on the stroke, the width and the seed.
QPainterPath brush_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed);

}  // namespace hpq
