#include "WatercolorPaint.h"

#include "GrainNoise.h"
#include "StrokeRibbon.h"
#include "WatercolorTexture.h"

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hpq {

namespace {

// The edge of a wash is blurred over this many logical pixels, on each side of the area.
constexpr double SoftEdgeRadius = 1.0;
// The pigment gathers at the edge over this many logical pixels inside the area.
constexpr double PoolRadius = 3.5;
// Each blur is two passes of a box: its reach is twice its radius.
constexpr double BlurReach = 2.0;

// A flat brush overruns the ends of its stroke by this many logical pixels: the bands of two
// cells that a grid line keeps apart join over it.
constexpr double BandOverrun = 1.5;

// The grain of the paper moves the edge of a wash by this part of its blurred width.
constexpr double EdgeFeather = 0.35;
// The coverage of the blurred area over which a wash goes from nothing to whole.
constexpr double EdgeStart = 0.30;
constexpr double EdgeEnd = 0.70;

// How much thicker than inside the wash is in the thin line right at its edge, and at the
// start of the wider rim that fades towards the inside.
constexpr double EdgeLineDensity = 0.85;
constexpr double RimDensity = 0.30;
// Inside, the wash is at most this much thicker or thinner than its brush laid it.
constexpr double WashSettling = 0.40;
// Along a line the pigment is at most this much thicker or thinner than its brush laid it.
constexpr double LineSettling = 0.80;
// The thickest wash that is told apart from thinner ones.
constexpr double MaxDensity = 3.0;
constexpr int DensityLevels = 256;

// The image of a line is cut in squares of this many pixels of the device, and only what the
// line covers of each square is kept: the frame of a window covers little of its rectangle.
constexpr int PieceSize = 64;

// What is laid on a window is kept, up to this many bytes: Qt repaints the whole window for
// a blinking cursor, and each wash or line is the same as long as it stays where it is.
constexpr int MaxCachedBytes = 48 * 1024 * 1024;

int radius_in_pixels(double radius, double device_pixel_ratio) {
    return std::max(1, static_cast<int>(std::lround(radius * device_pixel_ratio)));
}

// One value for each pixel of an image, row after row.
struct Field {
    int width = 0;
    int height = 0;
    std::vector<float> values;
};

enum class Direction { Rows, Columns };

// Replaces each value by the mean of the values up to `radius` pixels before and after it
// in its row or its column. Past the ends, the first and the last values are repeated.
void box_blur(Field &field, Direction direction, int radius) {
    const bool along_rows = direction == Direction::Rows;
    const int line_count = along_rows ? field.height : field.width;
    const int length = along_rows ? field.width : field.height;
    const std::ptrdiff_t line_stride = along_rows ? field.width : 1;
    const std::ptrdiff_t step = along_rows ? 1 : field.width;
    const float scale = 1.0f / static_cast<float>(2 * radius + 1);
    std::vector<float> line(static_cast<std::size_t>(length));
    for (int line_index = 0; line_index < line_count; ++line_index) {
        float *first = field.values.data() + line_index * line_stride;
        for (int index = 0; index < length; ++index) {
            line[index] = first[index * step];
        }
        float sum = 0.0f;
        for (int offset = -radius; offset <= radius; ++offset) {
            sum += line[std::clamp(offset, 0, length - 1)];
        }
        for (int index = 0; index < length; ++index) {
            first[index * step] = sum * scale;
            sum += line[std::min(index + radius + 1, length - 1)] - line[std::max(index - radius, 0)];
        }
    }
}

// Replaces each value by the largest of the values up to `radius` pixels before and after it
// in its row or its column.
void spread(Field &field, Direction direction, int radius) {
    const bool along_rows = direction == Direction::Rows;
    const int line_count = along_rows ? field.height : field.width;
    const int length = along_rows ? field.width : field.height;
    const std::ptrdiff_t line_stride = along_rows ? field.width : 1;
    const std::ptrdiff_t step = along_rows ? 1 : field.width;
    std::vector<float> line(static_cast<std::size_t>(length));
    for (int line_index = 0; line_index < line_count; ++line_index) {
        float *first = field.values.data() + line_index * line_stride;
        for (int index = 0; index < length; ++index) {
            line[index] = first[index * step];
        }
        for (int index = 0; index < length; ++index) {
            const int from = std::max(index - radius, 0);
            const int to = std::min(index + radius, length - 1);
            first[index * step] = *std::max_element(line.begin() + from, line.begin() + to + 1);
        }
    }
}

// Blurs `field` twice with a box, across the stroke of a band only: along a band, the wash
// ends where its area ends.
void blur(Field &field, int radius, WashKind kind, Direction across_band) {
    for (int pass = 0; pass < 2; ++pass) {
        if (kind == WashKind::Patch || across_band == Direction::Rows) {
            box_blur(field, Direction::Rows, radius);
        }
        if (kind == WashKind::Patch || across_band == Direction::Columns) {
            box_blur(field, Direction::Columns, radius);
        }
    }
}

// How much of each pixel of `device_rect` is in `area`, as the alpha of an image.
// The image has the format Qt paints fastest on, and not one that only holds the coverage.
QImage coverage_mask(const QPainterPath &area, const QRect &device_rect, double device_pixel_ratio) {
    QImage mask(device_rect.size(), QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(-device_rect.topLeft());
    painter.scale(device_pixel_ratio, device_pixel_ratio);
    painter.fillPath(area, Qt::white);
    painter.end();
    return mask;
}

// How much of each pixel of `device_rect` is in `area`, from 0 to 1.
Field coverage(const QPainterPath &area, const QRect &device_rect, double device_pixel_ratio) {
    const QImage mask = coverage_mask(area, device_rect, device_pixel_ratio);
    Field field{mask.width(), mask.height(), {}};
    field.values.reserve(static_cast<std::size_t>(mask.width()) * mask.height());
    for (int y = 0; y < mask.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(mask.constScanLine(y));
        for (int x = 0; x < mask.width(); ++x) {
            field.values.push_back(qAlpha(line[x]) / 255.0f);
        }
    }
    return field;
}

// The color of the pigment for each density that is told apart, on white paper.
std::array<QRgb, DensityLevels> pigment_colors(const QColor &pigment) {
    std::array<QRgb, DensityLevels> colors;
    for (int level = 0; level < DensityLevels; ++level) {
        colors[level] = laid_pigment(pigment, MaxDensity * level / (DensityLevels - 1));
    }
    return colors;
}

// The color of the pigment laid `density` times as thick as its brush lays it.
QRgb color_at(const std::array<QRgb, DensityLevels> &colors, double density) {
    const int level = static_cast<int>(std::lround(density / MaxDensity * (DensityLevels - 1)));
    return colors[std::clamp(level, 0, DensityLevels - 1)];
}

QRgb with_alpha(QRgb color, int alpha) {
    return qPremultiply(qRgba(qRed(color), qGreen(color), qBlue(color), alpha));
}

// A part of what a brush laid: an image with the resolution of the device, and where its
// first pixel is on the device.
struct Piece {
    QPoint origin;
    QImage image;
};

using Layer = std::vector<Piece>;

enum class LayerKind { Patch, Band, Line };

// What tells a layer from another one.
struct LayerKey {
    std::uint64_t area_hash;
    int element_count;
    QRgb pigment;
    LayerKind kind;
    QRect device_rect;
    double device_pixel_ratio;

    bool operator==(const LayerKey &other) const {
        return area_hash == other.area_hash && element_count == other.element_count && pigment == other.pigment &&
               kind == other.kind && device_rect == other.device_rect &&
               device_pixel_ratio == other.device_pixel_ratio;
    }
};

size_t qHash(const LayerKey &key, size_t seed = 0) {
    return qHashMulti(seed, key.area_hash, key.element_count, key.pigment, static_cast<int>(key.kind),
                      key.device_rect.x(), key.device_rect.y(), key.device_rect.width(), key.device_rect.height(),
                      key.device_pixel_ratio);
}

// A 64 bit FNV-1a hash of the fill rule and of the elements of `path`.
std::uint64_t path_hash(const QPainterPath &path) {
    std::uint64_t hash = 14695981039346656037ull;
    const auto add = [&hash](const void *data, std::size_t size) {
        const auto *bytes = static_cast<const unsigned char *>(data);
        for (std::size_t index = 0; index < size; ++index) {
            hash = (hash ^ bytes[index]) * 1099511628211ull;
        }
    };
    const int fill_rule = path.fillRule();
    add(&fill_rule, sizeof fill_rule);
    for (int index = 0; index < path.elementCount(); ++index) {
        const QPainterPath::Element element = path.elementAt(index);
        const int type = element.type;
        const double coordinates[] = {element.x, element.y};
        add(&type, sizeof type);
        add(coordinates, sizeof coordinates);
    }
    return hash;
}

// The layer of `key`, made by `make_layer` the first time it is asked for.
template <typename MakeLayer>
Layer cached_layer(const LayerKey &key, MakeLayer make_layer) {
    static QMutex mutex;
    static QCache<LayerKey, Layer> layers(MaxCachedBytes);
    const QMutexLocker locker(&mutex);
    if (const Layer *found = layers.object(key)) {
        return *found;
    }
    const Layer layer = make_layer();
    qsizetype bytes = 0;
    for (const Piece &piece : layer) {
        bytes += piece.image.sizeInBytes();
    }
    // A layer too large for the cache is not kept.
    layers.insert(key, new Layer(layer), static_cast<int>(std::min<qsizetype>(bytes, MaxCachedBytes + 1)));
    return layer;
}

// The area that `painter` can paint, in its logical coordinates; larger when it is not known.
QRectF paintable_area(const QPainter &painter) {
    const QPaintDevice *device = painter.device();
    // The size of a widget is in logical pixels and the one of an image in pixels of the
    // device: taking it for logical pixels is right for the first and too large for the second.
    const double ratio = device->devicePixelRatioF();
    const QRectF device_area(0, 0, device->width() * ratio, device->height() * ratio);
    const QRectF whole = painter.deviceTransform().inverted().mapRect(device_area);
    return painter.hasClipping() ? whole & painter.clipBoundingRect() : whole;
}

// The pixels of the device that a layer within `reach` logical pixels of `area` changes, among
// those `painter` can paint and those within `reach` of them.
QRect layer_rect(const QPainter &painter, const QPainterPath &area, double reach) {
    const double ratio = painter.device()->devicePixelRatioF();
    const QRectF touched = area.boundingRect().adjusted(-reach, -reach, reach, reach);
    const QRectF painted = touched & paintable_area(painter).adjusted(-reach, -reach, reach, reach);
    return QRectF(painted.topLeft() * ratio, painted.size() * ratio).toAlignedRect();
}

// The wash of `pigment` on `area`, for the pixels of `device_rect`.
QImage wash_image(const QPainterPath &area, const QColor &pigment, WashKind kind, const QRect &device_rect,
                  double device_pixel_ratio) {
    QImage image(device_rect.size(), QImage::Format_ARGB32_Premultiplied);
    const QRectF bounds = area.boundingRect();
    const Direction across_band = bounds.width() >= bounds.height() ? Direction::Columns : Direction::Rows;

    // The area with a soft edge, then blurred further: the further blur tells how far inside
    // the area a pixel is.
    Field soft = coverage(area, device_rect, device_pixel_ratio);
    if (kind == WashKind::Band) {
        const Direction along_band = across_band == Direction::Columns ? Direction::Rows : Direction::Columns;
        spread(soft, along_band, radius_in_pixels(BandOverrun, device_pixel_ratio));
    }
    blur(soft, radius_in_pixels(SoftEdgeRadius, device_pixel_ratio), kind, across_band);
    Field depth = soft;
    blur(depth, radius_in_pixels(PoolRadius, device_pixel_ratio), kind, across_band);

    const QImage settling = pigment_settling(device_pixel_ratio);
    const std::array<QRgb, DensityLevels> colors = pigment_colors(pigment);
    const double opacity = 255.0 * pigment.alphaF();
    for (int y = 0; y < image.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        const uchar *settling_line = settling.constScanLine(wrapped(device_rect.y() + y, settling.height()));
        const float *soft_line = soft.values.data() + static_cast<std::size_t>(y) * soft.width;
        const float *depth_line = depth.values.data() + static_cast<std::size_t>(y) * depth.width;
        for (int x = 0; x < image.width(); ++x) {
            const double level = settling_level(settling_line[wrapped(device_rect.x() + x, settling.width())]);
            const double cover = smooth_step(EdgeStart, EdgeEnd, soft_line[x] + EdgeFeather * level);
            if (cover <= 0.0) {
                line[x] = 0;
                continue;
            }
            const double edge_line = 1.0 - smooth_step(0.5, 1.0, soft_line[x]);
            const double rim = 1.0 - smooth_step(0.5, 1.0, depth_line[x]);
            const double density = 1.0 + WashSettling * level + EdgeLineDensity * edge_line + RimDensity * rim;
            line[x] = with_alpha(color_at(colors, density), static_cast<int>(std::lround(opacity * cover)));
        }
    }
    return image;
}

// The smallest part of `rect` that holds every pixel of `mask` that is not transparent in
// `rect`: an empty rectangle when there is none.
QRect covered_part(const QImage &mask, const QRect &rect) {
    int left = rect.right() + 1;
    int right = rect.left() - 1;
    int top = rect.bottom() + 1;
    int bottom = rect.top() - 1;
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(mask.constScanLine(y));
        for (int x = rect.left(); x <= rect.right(); ++x) {
            if (line[x] != 0) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = y;
            }
        }
    }
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

// The line of `pigment` that fills `ribbon`, for the pixels of `device_rect`: one piece for
// each square of the device that the line crosses.
Layer line_pieces(const QPainterPath &ribbon, const QColor &pigment, const QRect &device_rect,
                  double device_pixel_ratio) {
    const QImage mask = coverage_mask(ribbon, device_rect, device_pixel_ratio);
    const QImage settling = pigment_settling(device_pixel_ratio);
    const std::array<QRgb, DensityLevels> colors = pigment_colors(pigment);
    const double opacity = pigment.alphaF();
    Layer pieces;
    for (int top = 0; top < mask.height(); top += PieceSize) {
        for (int left = 0; left < mask.width(); left += PieceSize) {
            const QRect covered = covered_part(mask, QRect(left, top, PieceSize, PieceSize) & mask.rect());
            if (covered.isEmpty()) {
                continue;
            }
            const QPoint origin = device_rect.topLeft() + covered.topLeft();
            QImage image(covered.size(), QImage::Format_ARGB32_Premultiplied);
            for (int y = 0; y < image.height(); ++y) {
                auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
                const QRgb *mask_line =
                    reinterpret_cast<const QRgb *>(mask.constScanLine(covered.top() + y)) + covered.left();
                const uchar *settling_line = settling.constScanLine(wrapped(origin.y() + y, settling.height()));
                for (int x = 0; x < image.width(); ++x) {
                    const double level = settling_level(settling_line[wrapped(origin.x() + x, settling.width())]);
                    line[x] = with_alpha(color_at(colors, 1.0 + LineSettling * level),
                                         static_cast<int>(std::lround(opacity * qAlpha(mask_line[x]))));
                }
            }
            pieces.push_back({origin, image});
        }
    }
    return pieces;
}

// Lays on `painter` the layer of `kind` that `pigment` leaves on `area`, which changes the
// pixels within `reach` logical pixels of the area.
void paint_layer(QPainter &painter, const QPainterPath &area, const QColor &pigment, LayerKind kind, double reach) {
    const double ratio = painter.device()->devicePixelRatioF();
    const QRect device_rect = layer_rect(painter, area, reach);
    if (device_rect.isEmpty()) {
        return;
    }
    const LayerKey key{path_hash(area), area.elementCount(), pigment.rgba(), kind, device_rect, ratio};
    const Layer layer = cached_layer(key, [&] {
        Layer pieces = kind == LayerKind::Line
                           ? line_pieces(area, pigment, device_rect, ratio)
                           : Layer{{device_rect.topLeft(),
                                    wash_image(area, pigment, kind == LayerKind::Band ? WashKind::Band : WashKind::Patch,
                                               device_rect, ratio)}};
        for (Piece &piece : pieces) {
            piece.image.setDevicePixelRatio(ratio);
        }
        return pieces;
    });
    // A glaze: it darkens the paper and what was laid under it. Laid twice at the same place,
    // as Qt does for the selected row of a view, it does not darken itself.
    if (painter.paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
        painter.setCompositionMode(QPainter::CompositionMode_Darken);
    }
    for (const Piece &piece : layer) {
        painter.drawImage(QPointF(piece.origin) / ratio, piece.image);
    }
}

// The distance, in logical pixels, over which the edge of an area changes its wash.
constexpr double WashReach = BlurReach * (SoftEdgeRadius + PoolRadius) + BandOverrun + 1.0;
// A line changes the pixels its smooth edge touches.
constexpr double LineReach = 1.0;

}  // namespace

void paint_wash(QPainter &painter, const QPainterPath &area, const QColor &pigment, WashKind kind) {
    paint_layer(painter, area, pigment, kind == WashKind::Band ? LayerKind::Band : LayerKind::Patch, WashReach);
}

void paint_line(QPainter &painter, const QPainterPath &ribbon, const QColor &pigment) {
    paint_layer(painter, ribbon, pigment, LayerKind::Line, LineReach);
}

}  // namespace hpq
