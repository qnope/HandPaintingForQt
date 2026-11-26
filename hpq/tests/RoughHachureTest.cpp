#include <hpq/RoughGeometry.h>

#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

// The seed of the hand: the tests draw the same paths at every run.
constexpr std::uint32_t HandSeed = 2024;
constexpr std::uint32_t Seed = 7;
constexpr double Gap = 5.0;
constexpr double Tolerance = 1e-6;

// Options that draw exact hachures: nothing moves their ends.
hpq::RoughOptions exact_options(double angle)
{
    hpq::RoughOptions options;
    options.roughness = 0.0;
    options.hachure_gap = Gap;
    options.hachure_angle = angle;
    options.seed = HandSeed;
    return options;
}

hpq::RoughOptions rough_options()
{
    hpq::RoughOptions options;
    options.roughness = 1.0;
    options.max_offset = 5.0;
    options.hachure_gap = Gap;
    options.seed = HandSeed;
    return options;
}

QPainterPath rectangle_shape(const QRectF &rectangle)
{
    QPainterPath shape;
    shape.addRect(rectangle);
    return shape;
}

// The lines of a hachure path, each from its left end to its right end.
std::vector<QLineF> lines_of(const QPainterPath &hachure)
{
    std::vector<QLineF> lines;
    for (int i = 0; i + 1 < hachure.elementCount(); i += 2) {
        const QPainterPath::Element from = hachure.elementAt(i);
        const QPainterPath::Element to = hachure.elementAt(i + 1);
        EXPECT_EQ(from.type, QPainterPath::MoveToElement);
        EXPECT_EQ(to.type, QPainterPath::LineToElement);
        const QLineF line(QPointF(from.x, from.y), QPointF(to.x, to.y));
        lines.push_back(line.x1() <= line.x2() ? line : QLineF(line.p2(), line.p1()));
    }
    return lines;
}

std::vector<QLineF> hachure_lines(const QPainterPath &shape, const hpq::RoughOptions &options,
                                  std::uint32_t seed = Seed)
{
    return lines_of(hpq::rough_hachure(shape, options, seed));
}

// Distance from a point to the segment [a, b].
double distance_to_segment(QPointF point, QPointF a, QPointF b)
{
    const QPointF ab = b - a;
    const double length_squared = QPointF::dotProduct(ab, ab);
    const double t = length_squared == 0.0
                         ? 0.0
                         : std::clamp(QPointF::dotProduct(point - a, ab) / length_squared, 0.0, 1.0);
    return QLineF(point, a + t * ab).length();
}

// Distance from a point to the nearest side of a polygon.
double distance_to_boundary(QPointF point, const QPolygonF &polygon)
{
    double distance = distance_to_segment(point, polygon.last(), polygon.first());
    for (int i = 1; i < polygon.size(); ++i)
        distance = std::min(distance, distance_to_segment(point, polygon[i - 1], polygon[i]));
    return distance;
}

bool covers(const std::vector<QLineF> &lines, QPointF point)
{
    return std::any_of(lines.begin(), lines.end(), [point](const QLineF &line) {
        return std::abs(line.y1() - point.y()) < Gap / 2.0 && line.x1() <= point.x() && point.x() <= line.x2();
    });
}

// The y of the ends that the lines have on the vertical line at `x`, sorted.
std::vector<double> ends_at(const std::vector<QLineF> &lines, double x)
{
    std::vector<double> ends;
    for (const QLineF &line : lines) {
        for (const QPointF end : {line.p1(), line.p2()}) {
            if (std::abs(end.x() - x) < Tolerance)
                ends.push_back(end.y());
        }
    }
    std::sort(ends.begin(), ends.end());
    return ends;
}

}  // namespace

TEST(RoughHachureTest, HorizontalHachuresCrossTheRectangleOneGapApart)
{
    const QRectF rectangle(10, 10, 100, 60);

    std::vector<QLineF> lines = hachure_lines(rectangle_shape(rectangle), exact_options(0.0));

    ASSERT_EQ(lines.size(), 12u);
    std::sort(lines.begin(), lines.end(), [](const QLineF &a, const QLineF &b) { return a.y1() < b.y1(); });
    for (std::size_t i = 0; i < lines.size(); ++i) {
        EXPECT_NEAR(lines[i].x1(), rectangle.left(), Tolerance);
        EXPECT_NEAR(lines[i].x2(), rectangle.right(), Tolerance);
        EXPECT_NEAR(lines[i].y1(), lines[i].y2(), Tolerance);
        EXPECT_NEAR(lines[i].y1(), 12.5 + Gap * static_cast<double>(i), Tolerance);
    }
}

TEST(RoughHachureTest, HachuresFollowTheAngle)
{
    constexpr double Angle = -41.0;
    constexpr double DegreesPerRadian = 180.0 / 3.14159265358979323846;

    const std::vector<QLineF> lines =
        hachure_lines(rectangle_shape(QRectF(10, 10, 100, 60)), exact_options(Angle));

    ASSERT_FALSE(lines.empty());
    for (const QLineF &line : lines)
        EXPECT_NEAR(std::atan2(line.dy(), line.dx()) * DegreesPerRadian, Angle, 1e-3);
}

TEST(RoughHachureTest, EndsOfRoughHachuresMoveByAQuarterOfTheGapAtMost)
{
    // The ellipse is cut into sides before it is hatched: an end can be this far from the cut used here.
    constexpr double FlatteningTolerance = 0.5;
    QPainterPath shape;
    shape.addEllipse(QRectF(0, 0, 120, 80));
    const QPolygonF boundary = shape.toFillPolygon();

    const std::vector<QLineF> lines = hachure_lines(shape, rough_options());

    ASSERT_GT(lines.size(), 10u);
    for (const QLineF &line : lines) {
        for (const QPointF end : {line.p1(), line.p2()})
            EXPECT_LE(distance_to_boundary(end, boundary), Gap / 4.0 + FlatteningTolerance)
                << end.x() << "," << end.y();
    }
}

TEST(RoughHachureTest, HoleOfAnOddEvenShapeIsNotHatched)
{
    QPainterPath shape = rectangle_shape(QRectF(0, 0, 100, 100));
    shape.addRect(QRectF(30, 30, 40, 40));
    shape.setFillRule(Qt::OddEvenFill);

    const std::vector<QLineF> lines = hachure_lines(shape, exact_options(0.0));

    EXPECT_TRUE(covers(lines, QPointF(15, 48)));
    EXPECT_FALSE(covers(lines, QPointF(50, 48)));
    EXPECT_TRUE(covers(lines, QPointF(85, 48)));
}

TEST(RoughHachureTest, InnerShapeOfAWindingShapeIsHatched)
{
    QPainterPath shape = rectangle_shape(QRectF(0, 0, 100, 100));
    shape.addRect(QRectF(30, 30, 40, 40));
    shape.setFillRule(Qt::WindingFill);

    const std::vector<QLineF> lines = hachure_lines(shape, exact_options(0.0));

    EXPECT_TRUE(covers(lines, QPointF(50, 48)));
}

TEST(RoughHachureTest, ShapeNarrowerThanFourGapsIsNotHatched)
{
    EXPECT_TRUE(hpq::rough_hachure(rectangle_shape(QRectF(0, 0, 100, 4 * Gap - 1)), rough_options(), Seed).isEmpty());
    EXPECT_FALSE(hpq::rough_hachure(rectangle_shape(QRectF(0, 0, 100, 4 * Gap)), rough_options(), Seed).isEmpty());
}

TEST(RoughHachureTest, SameSeedGivesTheSameHachuresAndAnotherSeedOtherOnes)
{
    const QPainterPath shape = rectangle_shape(QRectF(10, 10, 100, 60));

    const QPainterPath first = hpq::rough_hachure(shape, rough_options(), Seed);

    EXPECT_EQ(first, hpq::rough_hachure(shape, rough_options(), Seed));
    EXPECT_NE(first, hpq::rough_hachure(shape, rough_options(), Seed + 1));
}

// The cells of a selected row are filled one by one: their hachures must look like one fill.
TEST(RoughHachureTest, HachuresOfAdjacentShapesLineUp)
{
    constexpr double SharedSide = 50.0;
    const hpq::RoughOptions options = exact_options(-41.0);

    const std::vector<double> left =
        ends_at(hachure_lines(rectangle_shape(QRectF(0, 0, SharedSide, 40)), options, 1), SharedSide);
    const std::vector<double> right =
        ends_at(hachure_lines(rectangle_shape(QRectF(SharedSide, 0, 60, 40)), options, 2), SharedSide);

    ASSERT_GT(left.size(), 3u);
    ASSERT_EQ(left.size(), right.size());
    for (std::size_t i = 0; i < left.size(); ++i)
        EXPECT_NEAR(left[i], right[i], Tolerance);
}
