#pragma once

#include <QBrush>
#include <QColor>
#include <QImage>

#include <optional>

namespace hpq {

// What the image of a tile brush shows.
enum class TileKind { ChalkStroke, ChalkText, ChalkFill, Slate, WatercolorPaper, WatercolorPigment };

// `tile`, an image that tiles, with `size` pixels on each side instead of its own, smoothed.
// It still tiles: it is scaled with its own repetitions around it.
QImage scaled_tile(const QImage &tile, int size);

// The number of pixels of a device of `device_pixel_ratio` along `logical_size` logical pixels.
int device_size(int logical_size, double device_pixel_ratio);

// The brush kept for `kind`, `color` and `device_pixel_ratio`; none when it was not made yet.
std::optional<QBrush> kept_tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio);

// Makes the brush of `kind` and `color` for a device of `device_pixel_ratio` from `tile`, an
// image that tiles, and keeps it.
QBrush keep_tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio, const QImage &tile);

// The brush of `kind` and `color` for a device of `device_pixel_ratio`: it paints the image
// that `make_tile` returns, which tiles, and has `color` as its color. The image is made the
// first time the brush is asked for, and kept. It keeps its size in logical pixels and has the
// resolution of the device, so that it is drawn as it is.
// The brush is anchored to the origin of the painter: its transform moves it elsewhere.
template <typename MakeTile>
QBrush tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio, MakeTile make_tile) {
    if (const std::optional<QBrush> kept = kept_tile_brush(kind, color, device_pixel_ratio)) {
        return *kept;
    }
    return keep_tile_brush(kind, color, device_pixel_ratio, make_tile());
}

}  // namespace hpq
