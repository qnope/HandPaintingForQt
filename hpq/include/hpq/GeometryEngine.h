#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>

#include <QPainterPath>

#include <cstdint>
#include <vector>

namespace hpq {

// Turns the shape of a primitive into the paths that are drawn instead of it.
// For a given engine, the result depends only on the shape, the options and the seed.
// A shape is given in the coordinates of the content of the widget being painted, not of the
// device: when a view scrolls, its shapes keep their paths.
class HPQ_EXPORT GeometryEngine {
public:
    GeometryEngine() = default;
    GeometryEngine(const GeometryEngine &) = default;
    GeometryEngine &operator=(const GeometryEngine &) = default;
    virtual ~GeometryEngine() = default;

    // The strokes that draw the outline of `shape`.
    virtual std::vector<QPainterPath> outline(const QPainterPath &shape, const RoughOptions &options,
                                              std::uint32_t seed) const = 0;

    // The strokes that draw a frame, kept out of it: where a stroke of the outline of `frame`
    // would go inside, it follows the edge of the frame. A frame around the content of a
    // widget leaves the content uncovered. By default, each stroke of the outline is united
    // with the frame.
    virtual std::vector<QPainterPath> frame_outline(const QPainterPath &frame, const RoughOptions &options,
                                                    std::uint32_t seed) const;

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
