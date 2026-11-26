#include "TileBrush.h"

#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <tuple>

namespace hpq {

namespace {

// The brushes of this many colors are kept; a window uses a handful of them.
constexpr std::size_t MaxCachedBrushes = 64;

// What a kept brush was made for: its kind, its color and the device pixel ratio.
using BrushKey = std::tuple<TileKind, QRgb, double>;

// The brushes made so far, and the mutex that guards them.
struct KeptBrushes {
    QMutex mutex;
    std::map<BrushKey, QBrush> brushes;
};

KeptBrushes &kept_brushes() {
    static KeptBrushes kept;
    return kept;
}

}  // namespace

QImage scaled_tile(const QImage &tile, int size) {
    if (size == tile.width()) {
        return tile;
    }
    QImage repeated(tile.size() * 3, QImage::Format_ARGB32_Premultiplied);
    repeated.fill(Qt::transparent);
    QPainter painter(&repeated);
    painter.fillRect(repeated.rect(), QBrush(tile));
    painter.end();
    return repeated.scaled(size * 3, size * 3, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .copy(size, size, size, size);
}

int device_size(int logical_size, double device_pixel_ratio) {
    return std::max(1, static_cast<int>(std::lround(logical_size * device_pixel_ratio)));
}

std::optional<QBrush> kept_tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio) {
    KeptBrushes &kept = kept_brushes();
    const QMutexLocker locker(&kept.mutex);
    const auto found = kept.brushes.find(BrushKey{kind, color.rgba(), device_pixel_ratio});
    return found != kept.brushes.end() ? std::optional<QBrush>(found->second) : std::nullopt;
}

QBrush keep_tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio, const QImage &tile) {
    const int size = device_size(tile.width(), device_pixel_ratio);
    QBrush brush(scaled_tile(tile, size));
    // The image has its own colors: the color of the brush only tells what it paints.
    brush.setColor(color);
    // The painter scales the brush by the device pixel ratio: one pixel of the image is one
    // pixel of the device.
    const double scale = static_cast<double>(tile.width()) / size;
    brush.setTransform(QTransform::fromScale(scale, scale));

    KeptBrushes &kept = kept_brushes();
    const QMutexLocker locker(&kept.mutex);
    if (kept.brushes.size() >= MaxCachedBrushes) {
        kept.brushes.clear();
    }
    kept.brushes.insert_or_assign(BrushKey{kind, color.rgba(), device_pixel_ratio}, brush);
    return brush;
}

}  // namespace hpq
