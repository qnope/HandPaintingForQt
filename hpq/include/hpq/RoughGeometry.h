#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

#include <QPainterPath>

#include <cstdint>
#include <vector>

namespace hpq {

// How hpq makes a shape rough: the paths that are drawn instead of it. Each stroke wobbles
// around the shape, never farther than RoughOptions::max_offset.
// A result depends only on the shape, on the options and on the seed of the shape: with the
// same RoughOptions::seed, a shape is drawn the same way every time, so a repaint changes
// nothing. Without wobble (a roughness or a max_offset of 0), the paths are the shape itself,
// exactly: each stroke of its outline, and the area of its fill.
// A shape is given in the coordinates of the content of the widget being painted, not of the
// device: when a view scrolls, its shapes keep their paths.

// The strokes that draw the outline of `shape`, RoughOptions::stroke_count of them.
HPQ_EXPORT std::vector<QPainterPath> rough_outline(const QPainterPath &shape, const RoughOptions &options,
                                                   std::uint32_t shape_seed);

// The strokes that draw a frame, kept out of it: where a stroke of the outline of `frame`
// would go inside, it follows the edge of the frame. A frame around the content of a widget
// leaves the content uncovered.
HPQ_EXPORT std::vector<QPainterPath> rough_frame_outline(const QPainterPath &frame, const RoughOptions &options,
                                                         std::uint32_t shape_seed);

// A closed area, close to `shape`, that is filled instead of it. It covers the whole shape:
// adjacent fills leave no unpainted seam between them.
HPQ_EXPORT QPainterPath rough_fill_area(const QPainterPath &shape, const RoughOptions &options,
                                        std::uint32_t shape_seed);

// The lines that hatch the inside of `shape`, all in one path: straight lines,
// RoughOptions::hachure_gap apart, whose ends move a little. They lie on a grid of the device,
// so that the hachures of two adjacent shapes line up. An empty path means that the shape
// cannot be hatched (it is too small, for example): it is filled instead.
HPQ_EXPORT QPainterPath rough_hachure(const QPainterPath &shape, const RoughOptions &options,
                                      std::uint32_t shape_seed);

}  // namespace hpq
