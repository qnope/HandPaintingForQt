#pragma once

#include <hpq/FwdDecls.h>

#include <QPainterPath>

#include <cstdint>
#include <vector>

namespace hpq {

// Turns the shape of a primitive into the paths that are drawn instead of it.
// The result depends only on the shape, the options and the seed.
class GeometryEngine {
public:
    GeometryEngine() = default;
    GeometryEngine(const GeometryEngine &) = default;
    GeometryEngine &operator=(const GeometryEngine &) = default;
    virtual ~GeometryEngine() = default;

    // The strokes that draw the outline of `shape`.
    virtual std::vector<QPainterPath> outline(const QPainterPath &shape, const RoughOptions &options,
                                              std::uint32_t seed) const = 0;
};

}  // namespace hpq
