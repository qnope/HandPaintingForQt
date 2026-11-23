#include "TileBrush.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QString>
#include <QTransform>

#include <algorithm>
#include <cmath>

namespace hpq {

namespace {

// The brushes of this many colors are kept; a window uses a handful of them.
constexpr int MaxCachedBrushes = 64;

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

QBrush tile_brush(TileKind kind, const QColor &color, double device_pixel_ratio,
                  const std::function<QImage()> &make_tile) {
    static QMutex mutex;
    static QHash<QString, QBrush> brushes;
    const QString key = QStringLiteral("%1/%2/%3").arg(static_cast<int>(kind)).arg(color.rgba()).arg(device_pixel_ratio);
    const QMutexLocker locker(&mutex);
    const auto found = brushes.constFind(key);
    if (found != brushes.constEnd()) {
        return *found;
    }
    if (brushes.size() >= MaxCachedBrushes) {
        brushes.clear();
    }
    const QImage tile = make_tile();
    const int size = device_size(tile.width(), device_pixel_ratio);
    QBrush brush(scaled_tile(tile, size));
    // The image has its own colors: the color of the brush only tells what it paints.
    brush.setColor(color);
    // The painter scales the brush by the device pixel ratio: one pixel of the image is one
    // pixel of the device.
    const double scale = static_cast<double>(tile.width()) / size;
    brush.setTransform(QTransform::fromScale(scale, scale));
    return *brushes.insert(key, brush);
}

}  // namespace hpq
