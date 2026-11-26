#include <hpq/ChinaInkTheme.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theme.h>
#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cstdint>
#include <string>
#include <vector>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;
const QSize SceneSize(240, 180);
const QColor LineColor(0x20, 0x40, 0x80);
const QColor FillColor(0xc0, 0x40, 0x40);
const QColor BandColor(0x9c, 0xcb, 0xe4);
// A part of the scene that cuts through its shapes.
const QRect Part(60, 30, 90, 80);
// The side, in pixels, of a square whose wash is too large to be kept.
constexpr int LargeSide = 1100;

struct NamedTheme {
    std::string name;
    hpq::Theme theme;
};

// One theme for each medium, with and without hachures.
std::vector<NamedTheme> themes() {
    return {{"sketch", hpq::sketch_theme(hpq::FillStyle::Solid)},
            {"sketch with hachures", hpq::sketch_theme(hpq::FillStyle::Hachure)},
            {"slate", hpq::slate_theme(hpq::FillStyle::Solid)},
            {"slate with hachures", hpq::slate_theme(hpq::FillStyle::Hachure)},
            {"China ink", hpq::china_ink_theme(hpq::FillStyle::Solid)},
            {"watercolor", hpq::watercolor_theme(hpq::FillStyle::Solid)},
            {"watercolor with hachures", hpq::watercolor_theme(hpq::FillStyle::Hachure)}};
}

// Lines of several widths, plain and dashed, an outlined fill, a frame, a band and a curve.
void draw_scene(QPainter &painter) {
    painter.setPen(QPen(LineColor, 1.0));
    painter.drawLine(QLineF(10, 12, 220, 12));
    painter.setPen(QPen(LineColor, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(QLineF(10, 24, 220, 30));
    painter.setPen(QPen(LineColor, 2.0, Qt::DashLine));
    painter.drawLine(QLineF(10, 40, 220, 40));

    painter.setPen(QPen(LineColor, 2.0));
    painter.setBrush(FillColor);
    painter.drawRect(QRectF(20, 55, 90, 50));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(125, 55, 90, 50));

    painter.setPen(Qt::NoPen);
    painter.setBrush(BandColor);
    painter.drawRect(QRectF(20, 115, 195, 18));

    painter.setPen(QPen(LineColor, 1.5));
    painter.setBrush(FillColor);
    painter.drawEllipse(QRectF(40, 140, 80, 30));
    QPainterPath curve(QPointF(140, 165));
    curve.cubicTo(QPointF(160, 130), QPointF(190, 180), QPointF(215, 145));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(curve);
}

// An image for a device of `device_pixel_ratio` that shows `size` logical pixels of white paper.
QImage paper(const QSize &size, double device_pixel_ratio) {
    QImage image(size * device_pixel_ratio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(device_pixel_ratio);
    image.fill(Qt::white);
    return image;
}

// The scene drawn by hand on a device of `device_pixel_ratio`, through `clip` when it is not
// empty, by a hand whose wobble comes from `base_seed`.
QImage drawn_scene(const hpq::Theme &theme, hpq::PaintCache *cache, double device_pixel_ratio,
                   const QRect &clip = QRect(), std::uint32_t base_seed = BaseSeed) {
    const hpq::RoughGeometryEngine geometry_engine(base_seed);
    QImage image = paper(SceneSize, device_pixel_ratio);
    QPainter target(&image);
    if (!clip.isEmpty()) {
        target.setClipRect(clip);
    }
    hpq::HandPaintDevice device(target, theme, geometry_engine, cache);
    QPainter painter(&device);
    draw_scene(painter);
    painter.end();
    target.end();
    return image;
}

}  // namespace

TEST(HandPaintCacheTest, DrawingWithACacheIsTheSameAsWithout) {
    for (const NamedTheme &named : themes()) {
        for (const double ratio : {1.0, 2.0}) {
            SCOPED_TRACE(named.name + " at " + std::to_string(ratio));
            hpq::PaintCache cache;

            const QImage without = drawn_scene(named.theme, nullptr, ratio);
            const QImage first = drawn_scene(named.theme, &cache, ratio);
            const QImage again = drawn_scene(named.theme, &cache, ratio);

            EXPECT_NE(without, paper(SceneSize, ratio));
            EXPECT_EQ(without, first);
            EXPECT_EQ(without, again);
        }
    }
}

TEST(HandPaintCacheTest, AShapeDrawnAgainIsNotMadeRoughAgain) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        hpq::PaintCache cache;

        drawn_scene(named.theme, &cache, 2.0);
        const std::size_t made_by_first_drawing = cache.made_count();
        drawn_scene(named.theme, &cache, 2.0);

        EXPECT_GT(made_by_first_drawing, 0u);
        EXPECT_EQ(cache.made_count(), made_by_first_drawing);
        EXPECT_GT(cache.byte_count(), 0u);
    }
}

// Nothing of a drawing is kept for the next one: the same scene drawn by another hand is
// another drawing.
TEST(HandPaintCacheTest, WithoutACacheEveryDrawingMakesItsShapes) {
    const hpq::Theme theme = hpq::sketch_theme();

    const QImage first = drawn_scene(theme, nullptr, 1.0, QRect(), BaseSeed);
    const QImage second = drawn_scene(theme, nullptr, 1.0, QRect(), BaseSeed + 1);

    EXPECT_NE(first, second);
    EXPECT_EQ(second, drawn_scene(theme, nullptr, 1.0, QRect(), BaseSeed + 1));
}

// A wide line is kept as an area for one device and as a stroke for another, and a wash has
// the pixels of its device: one cache serves both.
TEST(HandPaintCacheTest, OneCacheServesDevicesOfSeveralPixelRatios) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        hpq::PaintCache cache;

        for (const double ratio : {1.0, 2.0, 1.0, 1.5, 2.0}) {
            SCOPED_TRACE(ratio);
            EXPECT_EQ(drawn_scene(named.theme, &cache, ratio), drawn_scene(named.theme, nullptr, ratio));
        }
    }
}

// What is kept was made for the whole shape: a painter that paints a part of it, then another
// that paints all of it, draw what they draw without a cache.
TEST(HandPaintCacheTest, DrawingThroughAClipIsTheSameAsWithout) {
    for (const NamedTheme &named : themes()) {
        for (const double ratio : {1.0, 2.0}) {
            SCOPED_TRACE(named.name + " at " + std::to_string(ratio));
            hpq::PaintCache cache;

            const QImage part = drawn_scene(named.theme, &cache, ratio, Part);
            const QImage whole = drawn_scene(named.theme, &cache, ratio);

            EXPECT_EQ(part, drawn_scene(named.theme, nullptr, ratio, Part));
            EXPECT_EQ(whole, drawn_scene(named.theme, nullptr, ratio));
        }
    }
}

// The wash of a very large area is not kept: it is made for what the painter paints, as it is
// without a cache.
TEST(HandPaintCacheTest, AWashTooLargeToBeKeptIsDrawnTheSame) {
    const hpq::Theme theme = hpq::watercolor_theme();
    const hpq::RoughGeometryEngine engine(BaseSeed);
    hpq::PaintCache cache;
    const auto drawn = [&](hpq::PaintCache *used_cache) {
        QImage image = paper(QSize(LargeSide, LargeSide), 1.0);
        QPainter target(&image);
        hpq::HandPaintDevice device(target, theme, engine, used_cache);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(BandColor);
        painter.drawRect(QRectF(10, 10, LargeSide - 20, LargeSide - 20));
        painter.end();
        target.end();
        return image;
    };

    const QImage without = drawn(nullptr);
    const QImage with = drawn(&cache);

    EXPECT_NE(without, paper(QSize(LargeSide, LargeSide), 1.0));
    EXPECT_EQ(without, with);
    EXPECT_LT(cache.byte_count(), hpq::PaintCache::largest_kept_wash());
}
