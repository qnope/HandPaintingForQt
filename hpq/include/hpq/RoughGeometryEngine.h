#pragma once

#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>

namespace hpq {

// Geometry engine that gives shapes a hand-drawn look: each stroke wobbles
// around the original shape, never farther than RoughOptions::max_offset.
class RoughGeometryEngine : public GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const RoughOptions &options,
                                      std::uint32_t seed) const override;
};

}  // namespace hpq
