#pragma once

#include <QPainterPath>
#include <QRect>
#include <QTransform>

#include <vector>

namespace hpq {

// `path` cut in square tiles of the device that `to_device` maps it to, one path for each
// tile it reaches in the `paintable` pixels of the device, in the coordinates of `path` and with
// its fill rule. The tiles end between two pixels of the device: filled one after the other,
// with or without antialiasing, the pieces paint the paintable pixels that `path` paints, as
// `path` paints them.
std::vector<QPainterPath> path_tiles(const QPainterPath &path, const QTransform &to_device, const QRect &paintable);

}  // namespace hpq
