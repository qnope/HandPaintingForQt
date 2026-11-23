#include "WatercolorWash.h"

#include "GrainNoise.h"
#include "StrokeRibbon.h"
#include "WatercolorTexture.h"

#include <hpq/PaintCache.h>

#include <QImage>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
// The thickest wash that is told apart from thinner ones.
constexpr double MaxDensity = 3.0;
constexpr int DensityLevels = 256;

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

// The distance, in logical pixels, over which the edge of an area changes its wash.
constexpr double WashReach = BlurReach * (SoftEdgeRadius + PoolRadius) + BandOverrun + 1.0;

// The pixels of the device, counted from the origin of the painter, that hold `area`.
QRect device_rect_of(const QRectF &area, double device_pixel_ratio) {
    return QRectF(area.topLeft() * device_pixel_ratio, area.size() * device_pixel_ratio).toAlignedRect();
}

constexpr std::size_t BytesPerPixel = 4;

std::size_t byte_count_of(const QRect &device_rect) {
    return BytesPerPixel * static_cast<std::size_t>(device_rect.width()) * static_cast<std::size_t>(device_rect.height());
}

}  // namespace

void paint_wash(QPainter &painter, const QPainterPath &area, const QColor &pigment, WashKind kind,
                PaintCache *cache) {
    const double ratio = painter.device()->devicePixelRatioF();
    // The pixels of the device that the wash changes.
    const QRectF touched = area.boundingRect().adjusted(-WashReach, -WashReach, WashReach, WashReach);
    QRect device_rect = device_rect_of(touched, ratio);
    QImage wash;
    if (cache != nullptr && byte_count_of(device_rect) <= PaintCache::largest_kept_wash()) {
        // The whole wash, made once: the painter cuts what it cannot paint.
        wash = cache->wash(area, pigment.rgba64(), static_cast<int>(kind), ratio,
                           [&] { return wash_image(area, pigment, kind, device_rect, ratio); });
    } else {
        // Only the pixels that the painter can paint, and those close enough to change them.
        const QRectF painted = touched & paintable_area(painter).adjusted(-WashReach, -WashReach, WashReach, WashReach);
        device_rect = device_rect_of(painted, ratio);
        if (device_rect.isEmpty()) {
            return;
        }
        wash = wash_image(area, pigment, kind, device_rect, ratio);
    }
    wash.setDevicePixelRatio(ratio);
    // A glaze: it darkens the paper and what was laid under it. Laid twice at the same place,
    // as Qt does for the selected row of a view, it does not darken itself.
    if (painter.paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
        painter.setCompositionMode(QPainter::CompositionMode_Darken);
    }
    painter.drawImage(QPointF(device_rect.topLeft()) / ratio, wash);
}

}  // namespace hpq
