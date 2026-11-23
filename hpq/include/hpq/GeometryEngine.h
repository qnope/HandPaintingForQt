#pragma once

#include <hpq/FwdDecls.h>

#include <QPainterPath>

#include <cstdint>
#include <vector>

namespace hpq {

// Turns the shape of a primitive into the paths that are drawn instead of it.
// For a given engine, the result depends only on the shape, the options and the seed.
// A shape is given with the top left corner of its bounding rectangle at the origin, whatever
// its place on the device, and the paths are moved back to that place: a shape that moves keeps
// its paths.
class GeometryEngine {
public:
    GeometryEngine() = default;
    GeometryEngine(const GeometryEngine &) = default;
    GeometryEngine &operator=(const GeometryEngine &) = default;
    virtual ~GeometryEngine() = default;

    // The strokes that draw the outline of `shape`.
    virtual std::vector<QPainterPath> outline(const QPainterPath &shape, const RoughOptions &options,
                                              std::uint32_t seed) const = 0;

    // A closed area, close to `shape`, that is filled instead of it. It should cover the whole
    // shape, or adjacent fills leave unpainted seams between them.
    virtual QPainterPath fill_area(const QPainterPath &shape, const RoughOptions &options,
                                   std::uint32_t seed) const = 0;

    // The lines that hatch the inside of `shape`, all in one path. An empty path means that
    // the shape cannot be hatched (it is too small, for example): it is filled instead.
    virtual QPainterPath hachure(const QPainterPath &shape, const RoughOptions &options,
                                 std::uint32_t seed) const = 0;
};

}  // namespace hpq
