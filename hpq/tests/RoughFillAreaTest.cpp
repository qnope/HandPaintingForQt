#include <hpq/RoughGeometryEngine.h>

#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

constexpr std::uint32_t Seed = 7;

hpq::RoughOptions fill_options()
{
    hpq::RoughOptions options;
    options.roughness = 1.0;
    options.max_offset = 5.0;
    return options;
}

QPainterPath rectangle_shape()
{
    QPainterPath shape;
    shape.addRect(QRectF(10, 10, 100, 50));
    return shape;
}

QPainterPath ellipse_shape()
{
    QPainterPath shape;
    shape.addEllipse(QRectF(0, 0, 80, 40));
    return shape;
}

QPointF element_point(const QPainterPath &path, int index)
{
    const QPainterPath::Element element = path.elementAt(index);
    return {element.x, element.y};
}

// True when every subpath of `path` ends where it starts.
bool is_closed(const QPainterPath &path)
{
    if (path.elementCount() <= 2)
        return false;
    int start = 0;
    for (int i = 1; i <= path.elementCount(); ++i) {
        if (i < path.elementCount() && !path.elementAt(i).isMoveTo())
            continue;
        if (element_point(path, start) != element_point(path, i - 1))
            return false;
        start = i;
    }
    return true;
}

bool same_elements(const QPainterPath &a, const QPainterPath &b)
{
    if (a.elementCount() != b.elementCount())
        return false;
    for (int i = 0; i < a.elementCount(); ++i) {
        if (a.elementAt(i).type != b.elementAt(i).type || element_point(a, i) != element_point(b, i))
            return false;
    }
    return true;
}

void expect_rough_fill_of(const QPainterPath &shape, QPointF center)
{
    const hpq::RoughOptions options = fill_options();
    const QPainterPath fill = hpq::RoughGeometryEngine(BaseSeed).fill_area(shape, options, Seed);

    EXPECT_TRUE(is_closed(fill));
    const double margin = options.max_offset;
    const QRectF allowed = shape.boundingRect().adjusted(-margin, -margin, margin, margin);
    EXPECT_TRUE(allowed.contains(fill.boundingRect()));
    EXPECT_TRUE(fill.contains(center));
}

}  // namespace

TEST(RoughFillAreaTest, RectangleFillIsClosedNearTheShapeAndContainsItsCenter)
{
    expect_rough_fill_of(rectangle_shape(), QPointF(60, 35));
}

TEST(RoughFillAreaTest, EllipseFillIsClosedNearTheShapeAndContainsItsCenter)
{
    expect_rough_fill_of(ellipse_shape(), QPointF(40, 20));
}

TEST(RoughFillAreaTest, SameShapeOptionsAndSeedGiveTheSamePaths)
{
    const hpq::RoughGeometryEngine engine(BaseSeed);
    const QPainterPath shape = rectangle_shape();
    const hpq::RoughOptions options = fill_options();

    const std::vector<QPainterPath> first_outline = engine.outline(shape, options, Seed);
    const std::vector<QPainterPath> second_outline = engine.outline(shape, options, Seed);
    ASSERT_EQ(first_outline.size(), second_outline.size());
    for (std::size_t i = 0; i < first_outline.size(); ++i)
        EXPECT_TRUE(same_elements(first_outline[i], second_outline[i]));

    const QPainterPath first_fill = engine.fill_area(shape, options, Seed);
    const QPainterPath second_fill = engine.fill_area(shape, options, Seed);
    EXPECT_GT(first_fill.elementCount(), 0);
    EXPECT_TRUE(same_elements(first_fill, second_fill));
}

// Adjacent fills (for example the rectangles of a widget background region) must leave no
// unpainted seam between them: the fill area wobbles outward only, never inside the shape.
TEST(RoughFillAreaTest, FillAreaCoversTheWholeShape)
{
    constexpr double Step = 0.5;
    const QPainterPath shape = rectangle_shape();
    const QRectF inside = shape.boundingRect().adjusted(Step, Step, -Step, -Step);
    for (std::uint32_t seed = 0; seed < 10; ++seed) {
        const QPainterPath fill = hpq::RoughGeometryEngine(BaseSeed).fill_area(shape, fill_options(), seed);
        for (double y = inside.top(); y <= inside.bottom(); y += Step) {
            for (double x = inside.left(); x <= inside.right(); x += Step) {
                ASSERT_TRUE(fill.contains(QPointF(x, y))) << "seed " << seed << " point " << x << "," << y;
            }
        }
    }
}
