#pragma once

#include <QPainterPath>

#include <cstdint>

namespace hpq {

// The area a piece of chalk marks when it follows `stroke`, to be filled instead of stroking it.
// It is `width` pixels wide on average: the pressure of the hand makes it wider and narrower
// along the stroke, heavier where an open stroke starts than where it ends, and now and then
// the chalk skips and leaves a gap.
// The result depends only on the stroke, the width and the seed.
QPainterPath chalk_ribbon(const QPainterPath &stroke, double width, std::uint32_t seed);

}  // namespace hpq
