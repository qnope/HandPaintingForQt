#pragma once

#include <QPainterPath>
#include <QTransform>

#include <vector>

namespace hpq {

// `path` cut in square tiles of the device that `to_device` maps it to, one path for each
// tile it reaches, in the coordinates of `path` and with its fill rule. The tiles end between
// two pixels of the device: filled one after the other, with or without antialiasing, the
// pieces paint the pixels that `path` paints, as `path` paints them.
std::vector<QPainterPath> path_tiles(const QPainterPath &path, const QTransform &to_device);

}  // namespace hpq
