#include <hpq/PaintCache.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainterPath>

#include <cstddef>
#include <vector>

namespace {

using hpq::PaintCache;

constexpr int Tool = 1;
constexpr double Width = 2.0;
constexpr std::uint64_t Pigment = 0x1234;
constexpr int WashKind = 0;
constexpr double PixelRatio = 2.0;
// The cache holds two generations of at most 4 megabytes of paths and 16 megabytes of washes.
constexpr std::size_t MostPathBytes = 2 * 4 * 1024 * 1024;
constexpr std::size_t MostWashBytes = 2 * 16 * 1024 * 1024;
// Enough shapes, of about ten kilobytes of paths each, to fill the cache several times.
constexpr int ManyShapes = 4000;
constexpr int PointsOfLongPath = 400;

QPainterPath rectangle_at(double x, double y = 0.0) {
    QPainterPath shape;
    shape.addRect(QRectF(x, y, 30.0, 20.0));
    return shape;
}

QPainterPath long_path() {
    QPainterPath path;
    for (int point = 0; point < PointsOfLongPath; ++point) {
        path.lineTo(point, point % 7);
    }
    return path;
}

// Counts how many times the cache asked for the paths, and returns `paths` each time.
class PathMaker {
public:
    explicit PathMaker(std::vector<QPainterPath> paths) : m_paths(std::move(paths)) {}

    std::vector<QPainterPath> get(PaintCache &cache, PaintCache::PathKind kind, const QPainterPath &shape,
                                  int tool = Tool, double width = Width) {
        return cache.paths(kind, shape, tool, width, [this] {
            ++m_call_count;
            return m_paths;
        });
    }

    int call_count() const { return m_call_count; }

private:
    std::vector<QPainterPath> m_paths;
    int m_call_count = 0;
};

}  // namespace

TEST(PaintCacheTest, PathsOfAShapeAreMadeOnce) {
    PaintCache cache;
    PathMaker maker({rectangle_at(1.0), rectangle_at(2.0)});

    const std::vector<QPainterPath> first = maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0));
    const std::vector<QPainterPath> second = maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0));

    EXPECT_EQ(maker.call_count(), 1);
    EXPECT_EQ(first, second);
    ASSERT_EQ(second.size(), 2u);
    EXPECT_EQ(second[0], rectangle_at(1.0));
    EXPECT_EQ(second[1], rectangle_at(2.0));
}

TEST(PaintCacheTest, AnEmptyListOfPathsIsKeptToo) {
    PaintCache cache;
    PathMaker maker({});

    maker.get(cache, PaintCache::PathKind::Hachure, rectangle_at(10.0));
    const std::vector<QPainterPath> second = maker.get(cache, PaintCache::PathKind::Hachure, rectangle_at(10.0));

    EXPECT_EQ(maker.call_count(), 1);
    EXPECT_TRUE(second.empty());
}

TEST(PaintCacheTest, AnotherShapeKindToolOrWidthGetsItsOwnPaths) {
    PaintCache cache;
    PathMaker maker({rectangle_at(1.0)});
    const QPainterPath shape = rectangle_at(10.0);
    maker.get(cache, PaintCache::PathKind::Outline, shape);

    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(11.0));
    EXPECT_EQ(maker.call_count(), 2);
    maker.get(cache, PaintCache::PathKind::FrameOutline, shape);
    EXPECT_EQ(maker.call_count(), 3);
    maker.get(cache, PaintCache::PathKind::Outline, shape, Tool + 1);
    EXPECT_EQ(maker.call_count(), 4);
    maker.get(cache, PaintCache::PathKind::Outline, shape, Tool, Width + 0.5);
    EXPECT_EQ(maker.call_count(), 5);

    // Each of them is kept.
    maker.get(cache, PaintCache::PathKind::Outline, shape);
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(11.0));
    maker.get(cache, PaintCache::PathKind::FrameOutline, shape);
    maker.get(cache, PaintCache::PathKind::Outline, shape, Tool + 1);
    maker.get(cache, PaintCache::PathKind::Outline, shape, Tool, Width + 0.5);
    EXPECT_EQ(maker.call_count(), 5);
}

// QPainterPath compares coordinates loosely: the cache does not.
TEST(PaintCacheTest, AShapeMovedByAHairIsAnotherShape) {
    PaintCache cache;
    PathMaker maker({rectangle_at(1.0)});
    ASSERT_EQ(rectangle_at(10.0), rectangle_at(10.0 + 1e-13));

    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0));
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0 + 1e-13));

    EXPECT_EQ(maker.call_count(), 2);
}

TEST(PaintCacheTest, ShapesWithAnotherFillRuleAreToldApart) {
    PaintCache cache;
    PathMaker maker({rectangle_at(1.0)});
    QPainterPath winding = rectangle_at(10.0);
    winding.setFillRule(Qt::WindingFill);

    maker.get(cache, PaintCache::PathKind::FillArea, rectangle_at(10.0));
    maker.get(cache, PaintCache::PathKind::FillArea, winding);

    EXPECT_EQ(maker.call_count(), 2);
}

TEST(PaintCacheTest, AWashIsMadeOnce) {
    PaintCache cache;
    int call_count = 0;
    const auto make = [&call_count] {
        ++call_count;
        QImage image(8, 4, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        return image;
    };

    const QImage first = cache.wash(rectangle_at(10.0), Pigment, WashKind, PixelRatio, make);
    const QImage second = cache.wash(rectangle_at(10.0), Pigment, WashKind, PixelRatio, make);

    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(first, second);
    EXPECT_EQ(second.size(), QSize(8, 4));
}

TEST(PaintCacheTest, AnotherAreaPigmentKindOrPixelRatioGetsItsOwnWash) {
    PaintCache cache;
    int call_count = 0;
    const auto make = [&call_count] {
        ++call_count;
        QImage image(8, 4, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        return image;
    };
    const QPainterPath area = rectangle_at(10.0);
    cache.wash(area, Pigment, WashKind, PixelRatio, make);

    cache.wash(rectangle_at(11.0), Pigment, WashKind, PixelRatio, make);
    cache.wash(area, Pigment + 1, WashKind, PixelRatio, make);
    cache.wash(area, Pigment, WashKind + 1, PixelRatio, make);
    cache.wash(area, Pigment, WashKind, 1.0, make);
    EXPECT_EQ(call_count, 5);

    cache.wash(area, Pigment, WashKind, PixelRatio, make);
    EXPECT_EQ(call_count, 5);
}

// Paths and washes are kept apart: the paths of a shape are not the wash of the same shape.
TEST(PaintCacheTest, PathsAndWashesOfTheSameShapeAreKeptApart) {
    PaintCache cache;
    PathMaker maker({rectangle_at(1.0)});
    int wash_count = 0;
    const QPainterPath shape = rectangle_at(10.0);

    maker.get(cache, PaintCache::PathKind::Outline, shape, 0, PixelRatio);
    cache.wash(shape, 0, 0, PixelRatio, [&wash_count] {
        ++wash_count;
        QImage image(8, 4, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        return image;
    });

    EXPECT_EQ(maker.call_count(), 1);
    EXPECT_EQ(wash_count, 1);
}

TEST(PaintCacheTest, ItCountsWhatItKeepsAndForgetsItWhenCleared) {
    PaintCache cache;
    EXPECT_EQ(cache.byte_count(), 0u);
    PathMaker maker({long_path()});
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0));
    const std::size_t with_paths = cache.byte_count();
    EXPECT_GE(with_paths, static_cast<std::size_t>(PointsOfLongPath) * 3 * sizeof(double));

    QImage image(100, 50, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    cache.wash(rectangle_at(10.0), Pigment, WashKind, PixelRatio, [&image] { return image; });
    EXPECT_GE(cache.byte_count(), with_paths + static_cast<std::size_t>(image.sizeInBytes()));

    cache.clear();
    EXPECT_EQ(cache.byte_count(), 0u);
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(10.0));
    EXPECT_EQ(maker.call_count(), 2);
}

TEST(PaintCacheTest, ItStaysSmallWhenShapesNeverComeBack) {
    PaintCache cache;
    PathMaker maker({long_path()});

    for (int index = 0; index < ManyShapes; ++index) {
        maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(index));
        EXPECT_LE(cache.byte_count(), MostPathBytes);
    }

    // The first shapes were forgotten, the last ones are kept.
    EXPECT_GT(static_cast<std::size_t>(ManyShapes) * PointsOfLongPath * 3 * sizeof(double), MostPathBytes);
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(ManyShapes - 1));
    EXPECT_EQ(maker.call_count(), ManyShapes);
    maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(0));
    EXPECT_EQ(maker.call_count(), ManyShapes + 1);
}

// What is drawn at every repaint stays, whatever else is drawn once and never again.
TEST(PaintCacheTest, AShapeDrawnAgainAndAgainIsNeverForgotten) {
    PaintCache cache;
    PathMaker kept_maker({long_path()});
    PathMaker other_maker({long_path()});
    const QPainterPath kept_shape = rectangle_at(0.0, 100.0);

    for (int index = 0; index < ManyShapes; ++index) {
        kept_maker.get(cache, PaintCache::PathKind::Outline, kept_shape);
        other_maker.get(cache, PaintCache::PathKind::Outline, rectangle_at(index));
    }

    EXPECT_EQ(kept_maker.call_count(), 1);
    EXPECT_EQ(other_maker.call_count(), ManyShapes);
    EXPECT_LE(cache.byte_count(), MostPathBytes);
}

TEST(PaintCacheTest, WashesStayUnderTheirOwnLimit) {
    PaintCache cache;
    // A megabyte each.
    QImage image(512, 512, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    ASSERT_LE(static_cast<std::size_t>(image.sizeInBytes()), PaintCache::largest_kept_wash());

    for (int index = 0; index < 100; ++index) {
        cache.wash(rectangle_at(index), Pigment, WashKind, PixelRatio, [&image] { return image; });
        EXPECT_LE(cache.byte_count(), MostWashBytes);
    }
    EXPECT_GT(cache.byte_count(), MostWashBytes / 2);
}
