#include "RecordingPaintDevice.h"

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPainter>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QRectF Rectangle(10, 10, 100, 50);

// Geometry engine that returns the shape unchanged.
class IdentityGeometryEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &,
                                      std::uint32_t) const override {
        return {shape};
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        return shape;
    }
};

// Draws `rectangle` with a pen and a brush on a HandPaintDevice, in its own painting session,
// after setting `transform` on the painter, and returns the calls the target received.
std::vector<DrawCall> draw_rectangle(const QRectF &rectangle, const hpq::GeometryEngine &geometry_engine,
                                     const QTransform &transform = QTransform()) {
    const hpq::Theme theme = hpq::sketch_theme();
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black, 2.0));
    painter.setBrush(QBrush(Qt::blue));
    painter.setTransform(transform);
    painter.drawRect(rectangle);
    painter.end();
    return target.engine().calls();
}

std::vector<QPainterPath> paths_of(const std::vector<DrawCall> &calls) {
    std::vector<QPainterPath> paths;
    for (const DrawCall &call : calls) {
        paths.push_back(call.path);
    }
    return paths;
}

std::vector<QPainterPath> translated(std::vector<QPainterPath> paths, const QPointF &offset) {
    for (QPainterPath &path : paths) {
        path.translate(offset);
    }
    return paths;
}

void expect_same_elements(const QPainterPath &first, const QPainterPath &second) {
    ASSERT_EQ(first.elementCount(), second.elementCount());
    for (int index = 0; index < first.elementCount(); ++index) {
        const QPainterPath::Element a = first.elementAt(index);
        const QPainterPath::Element b = second.elementAt(index);
        EXPECT_EQ(a.type, b.type);
        EXPECT_DOUBLE_EQ(a.x, b.x);
        EXPECT_DOUBLE_EQ(a.y, b.y);
    }
}

void expect_same_paths(const std::vector<QPainterPath> &first, const std::vector<QPainterPath> &second) {
    ASSERT_EQ(first.size(), second.size());
    ASSERT_FALSE(first.empty());
    for (std::size_t index = 0; index < first.size(); ++index) {
        expect_same_elements(first[index], second[index]);
    }
}

TEST(HandPaintSeedTest, IdentityEngineMakesTargetReceiveTheExactRectangle) {
    const IdentityGeometryEngine geometry_engine;
    QPainterPath exact_rectangle;
    exact_rectangle.addRect(Rectangle);

    const std::vector<DrawCall> calls = draw_rectangle(Rectangle, geometry_engine);

    ASSERT_GE(calls.size(), 2u);
    EXPECT_TRUE(calls[0].is_fill());
    for (std::size_t index = 1; index < calls.size(); ++index) {
        EXPECT_TRUE(calls[index].is_stroke());
    }
    for (const DrawCall &call : calls) {
        EXPECT_EQ(call.path, exact_rectangle);
    }
}

TEST(HandPaintSeedTest, SameRectangleDrawnTwiceGivesTheSamePaths) {
    const hpq::RoughGeometryEngine geometry_engine;

    const std::vector<QPainterPath> first = paths_of(draw_rectangle(Rectangle, geometry_engine));
    const std::vector<QPainterPath> second = paths_of(draw_rectangle(Rectangle, geometry_engine));

    expect_same_paths(first, second);
}

TEST(HandPaintSeedTest, SeedUsesDeviceCoordinatesOnce) {
    const hpq::RoughGeometryEngine geometry_engine;
    const QPointF offset(100, 100);

    const std::vector<QPainterPath> moved = paths_of(draw_rectangle(Rectangle.translated(offset), geometry_engine));
    const std::vector<QPainterPath> transformed = paths_of(
        draw_rectangle(Rectangle, geometry_engine, QTransform::fromTranslate(offset.x(), offset.y())));

    // Both rectangles are at the same place on the device, so they get the same seed.
    expect_same_paths(moved, transformed);
}

TEST(HandPaintSeedTest, DifferentRectanglesGiveDifferentPaths) {
    const hpq::RoughGeometryEngine geometry_engine;
    const QPointF offset(20, 30);

    const std::vector<QPainterPath> first = paths_of(draw_rectangle(Rectangle, geometry_engine));
    const std::vector<QPainterPath> second = paths_of(draw_rectangle(Rectangle.translated(offset), geometry_engine));

    // Moved back onto the first rectangle, the rough paths still differ: they come from another seed.
    EXPECT_NE(first, translated(second, -offset));
}

}  // namespace
