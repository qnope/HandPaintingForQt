#pragma once

#include <QPainterPath>
#include <QRect>
#include <QSizeF>
#include <QTransform>

#include <vector>

namespace hpq {

// How a path is cut in pieces. The raster engine of Qt smooths a path in bands of rows, and
// keeps the pixels that its edges cross in a buffer of a few hundred: when a band crosses more,
// the engine halves the band, or starts the whole path again with a larger buffer. The pieces
// cross few enough pixels in each row.
enum class Tiling {
    // A stroke, long and thin, crosses pixels all along each row it is in: in square tiles.
    Stroke,
    // An area crosses two pixels in most rows, and many only along its top and bottom: in
    // strips side by side, which go from its top to its bottom.
    Area,
};

// Whether a path whose bounds on the device have `size` fits in one piece of `tiling`.
bool fits_in_a_piece(const QSizeF &size, Tiling tiling);

// `path` cut in the pieces of `tiling`, on the device that `to_device` maps it to: one path for
// each piece that reaches the `paintable` pixels of the device, in the coordinates of `path` and
// with its fill rule; `path` itself when it is small enough. The pieces end between two pixels
// of the device: filled one after the other, with or without antialiasing, they paint the
// paintable pixels that `path` paints, as `path` paints them.
std::vector<QPainterPath> path_tiles(const QPainterPath &path, const QTransform &to_device, const QRect &paintable,
                                     Tiling tiling);

}  // namespace hpq
