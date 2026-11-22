#include "ChalkTexture.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QString>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hpq {

namespace {

// Side, in pixels, of the grain images. The cells of every noise below divide it, so that the
// images tile without a seam.
constexpr int GrainSize = 128;
// Side, in pixels, of the slate image: larger, so that its clouds do not visibly repeat.
constexpr int SlateSize = 256;
// The brushes of this many colors are kept; a window uses a handful of them.
constexpr int MaxCachedBrushes = 64;

// A number in [0, 1) that depends only on the lattice point and the salt.
double lattice_value(int x, int y, std::uint32_t salt) {
    std::uint32_t hash = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u ^
                         salt * 83492791u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    hash ^= hash >> 16;
    return hash / 4294967296.0;
}

int wrapped(int value, int period) {
    const int remainder = value % period;
    return remainder < 0 ? remainder + period : remainder;
}

double smooth(double t) {
    return t * t * (3.0 - 2.0 * t);
}

// Value noise in [0, 1): random values on a lattice of `cell_x` by `cell_y` cells, blended
// smoothly between them. The lattice repeats after `period_x` by `period_y` cells.
double value_noise(double x, double y, double cell_x, double cell_y, int period_x, int period_y,
                   std::uint32_t salt) {
    const double lattice_x = x / cell_x;
    const double lattice_y = y / cell_y;
    const int left = static_cast<int>(std::floor(lattice_x));
    const int top = static_cast<int>(std::floor(lattice_y));
    const double tx = smooth(lattice_x - left);
    const double ty = smooth(lattice_y - top);
    const auto corner = [&](int dx, int dy) {
        return lattice_value(wrapped(left + dx, period_x), wrapped(top + dy, period_y), salt);
    };
    const double upper = (1.0 - tx) * corner(0, 0) + tx * corner(1, 0);
    const double lower = (1.0 - tx) * corner(0, 1) + tx * corner(1, 1);
    return (1.0 - ty) * upper + ty * lower;
}

// Square value noise of `cell` pixels that repeats every `size` pixels.
double square_noise(int x, int y, int cell, int size, std::uint32_t salt) {
    return value_noise(x, y, cell, cell, size / cell, size / cell, salt);
}

// How strongly the tip of the chalk marks the pixel: a different value for each pixel, small
// pits two pixels wide and bare patches eight pixels wide.
double stroke_field(int x, int y) {
    return 0.50 * lattice_value(x, y, 1) + 0.38 * square_noise(x, y, 2, GrainSize, 2) +
           0.12 * square_noise(x, y, 8, GrainSize, 3);
}

// How strongly the side of the chalk marks the pixel: long thin streaks along one diagonal
// over the grain of the slate.
double fill_field(int x, int y) {
    constexpr double StreakLength = 16.0;
    constexpr double StreakWidth = 2.0;
    const double streak = value_noise(x + y, x - y, StreakLength, StreakWidth,
                                      static_cast<int>(GrainSize / StreakLength),
                                      static_cast<int>(GrainSize / StreakWidth), 4);
    return 0.35 * lattice_value(x, y, 5) + 0.40 * streak + 0.25 * square_noise(x, y, 4, GrainSize, 6);
}

// The Alpha8 image where the `coverage` part of the pixels with the highest `field` is covered.
// `softness` is the range of field values over which a pixel goes from bare to covered.
template <typename Field>
QImage grain_image(Field field, double coverage, double softness) {
    std::vector<double> values;
    values.reserve(GrainSize * GrainSize);
    for (int y = 0; y < GrainSize; ++y) {
        for (int x = 0; x < GrainSize; ++x) {
            values.push_back(field(x, y));
        }
    }
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const auto threshold_index = static_cast<std::size_t>((1.0 - coverage) * (sorted.size() - 1));
    const double threshold = sorted[threshold_index];

    QImage image(GrainSize, GrainSize, QImage::Format_Alpha8);
    for (int y = 0; y < GrainSize; ++y) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < GrainSize; ++x) {
            const double alpha = std::clamp(0.5 + (values[y * GrainSize + x] - threshold) / softness, 0.0, 1.0);
            line[x] = static_cast<uchar>(std::lround(alpha * 255.0));
        }
    }
    return image;
}

// The color of the dust left on a slate that was written on and wiped.
const QColor DustColor(0xe8, 0xe4, 0xd8);

// The part of dust over the slate at a pixel: wide clouds, smaller smudges and a fine grain.
double dust_amount(int x, int y) {
    return 0.07 * square_noise(x, y, 64, SlateSize, 7) + 0.05 * square_noise(x, y, 16, SlateSize, 8) +
           0.035 * lattice_value(x, y, 9);
}

int blended(int from, int to, double amount) {
    return static_cast<int>(std::lround(from + (to - from) * amount));
}

QImage slate_image(const QColor &color) {
    QImage image(SlateSize, SlateSize, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < SlateSize; ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < SlateSize; ++x) {
            const double dust = dust_amount(x, y);
            const QRgb pixel = qRgba(blended(color.red(), DustColor.red(), dust),
                                     blended(color.green(), DustColor.green(), dust),
                                     blended(color.blue(), DustColor.blue(), dust), color.alpha());
            line[x] = qPremultiply(pixel);
        }
    }
    return image;
}

// `color` where the grain is covered, transparent elsewhere.
QImage tinted(const QImage &grain, const QColor &color) {
    QImage image(grain.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.drawImage(0, 0, grain);
    return image;
}

// `tile`, an image that tiles, with `size` pixels on each side instead of its own, smoothed.
// It still tiles: it is scaled with its own repetitions around it.
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

enum class BrushKind { ChalkStroke, ChalkText, ChalkFill, Slate };

// The brush of `kind` and `color` for a device of `device_pixel_ratio`. Its image is the one
// made by `make_tile`, the first time the brush is asked for, at the resolution of the device.
template <typename MakeTile>
QBrush cached_brush(BrushKind kind, const QColor &color, double device_pixel_ratio, MakeTile make_tile) {
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
    const int size = std::max(1, static_cast<int>(std::lround(tile.width() * device_pixel_ratio)));
    QBrush brush(scaled_tile(tile, size));
    // The image has its own colors: the color of the brush only tells what it paints.
    brush.setColor(color);
    // The painter scales the brush by the device pixel ratio: one pixel of the image is one
    // pixel of the device.
    const double scale = static_cast<double>(tile.width()) / size;
    brush.setTransform(QTransform::fromScale(scale, scale));
    return *brushes.insert(key, brush);
}

BrushKind brush_kind(ChalkGrain grain) {
    switch (grain) {
    case ChalkGrain::Stroke:
        return BrushKind::ChalkStroke;
    case ChalkGrain::Text:
        return BrushKind::ChalkText;
    default:
        return BrushKind::ChalkFill;
    }
}

}  // namespace

QImage chalk_grain(ChalkGrain grain) {
    constexpr double StrokeCoverage = 0.82;
    constexpr double TextCoverage = 0.93;
    constexpr double StrokeSoftness = 0.10;
    constexpr double FillCoverage = 0.50;
    constexpr double FillSoftness = 0.16;
    static const QImage stroke_grain = grain_image(stroke_field, StrokeCoverage, StrokeSoftness);
    static const QImage text_grain = grain_image(stroke_field, TextCoverage, StrokeSoftness);
    static const QImage fill_grain = grain_image(fill_field, FillCoverage, FillSoftness);
    switch (grain) {
    case ChalkGrain::Stroke:
        return stroke_grain;
    case ChalkGrain::Text:
        return text_grain;
    default:
        return fill_grain;
    }
}

QBrush chalk_brush(const QColor &color, ChalkGrain grain, double device_pixel_ratio) {
    return cached_brush(brush_kind(grain), color, device_pixel_ratio,
                        [&] { return tinted(chalk_grain(grain), color); });
}

QBrush slate_brush(const QColor &color, double device_pixel_ratio) {
    return cached_brush(BrushKind::Slate, color, device_pixel_ratio, [&] { return slate_image(color); });
}

}  // namespace hpq
