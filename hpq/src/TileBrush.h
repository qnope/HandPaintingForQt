#pragma once

#include <QBrush>
#include <QColor>
#include <QImage>

#include <functional>

namespace hpq {

// What the image of a tile brush shows.
enum class TileKind { ChalkStroke, ChalkText, ChalkFill, Slate, WatercolorPaper };

// `tile`, an image that tiles, with `size` pixels on each side instead of its own, smoothed.
// It still tiles: it is scaled with its own repetitions around it.
QImage scaled_tile(const QImage &tile, int size);

// The number of pixels of a device of `device_pixel_ratio` along `logical_size` logical pixels.
int device_size(int logical_size, double device_pixel_ratio);

// The brush of `kind` and `color` for a device of `device_pixel_ratio`: it paints the image
// made by `make_tile`, which tiles, and has `color` as its color. The image is made the first
// time the brush is asked for, and kept. It keeps its size in logical pixels and has the
// resolution of the device, so that it is drawn as it is.
// The brush is anchored to the origin of the painter, not to what is drawn.
QBrush tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio,
                  const std::function<QImage()> &make_tile);

}  // namespace hpq
