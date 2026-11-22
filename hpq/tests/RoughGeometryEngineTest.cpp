#include <hpq/RoughGeometryEngine.h>

#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace {

QPainterPath line_path(QPointF from, QPointF to)
{
    QPainterPath path(from);
    path.lineTo(to);
    return path;
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

double distance(QPointF a, QPointF b) { return QLineF(a, b).length(); }

QPointF element_point(const QPainterPath &path, int index)
{
    const QPainterPath::Element element = path.elementAt(index);
    return {element.x, element.y};
}

QPointF first_point(const QPainterPath &path) { return element_point(path, 0); }

QPointF last_point(const QPainterPath &path) { return element_point(path, path.elementCount() - 1); }

hpq::RoughOptions rough_options(double roughness)
{
    hpq::RoughOptions options;
    options.roughness = roughness;
    options.stroke_count = 2;
    options.max_offset = 5.0;
    return options;
}

std::vector<QPainterPath> rough_line_outline(double roughness, std::uint32_t seed)
{
    const hpq::RoughGeometryEngine engine;
    return engine.outline(line_path({0, 0}, {100, 0}), rough_options(roughness), seed);
}

const QPointF LineStart(0, 0);
const QPointF LineEnd(100, 0);

// An offset of exactly max_offset can measure a few ulps more, depending on the platform.
const double RoundingTolerance = 1e-9;

}  // namespace

TEST(RoughGeometryEngine, LineOutlineHasStrokeCountStrokesNearTheSegment)
{
    const double max_offset = 5.0;
    const std::vector<QPainterPath> strokes = rough_line_outline(1.0, 42);

    ASSERT_EQ(2u, strokes.size());
    for (const QPainterPath &stroke : strokes) {
        ASSERT_GE(stroke.elementCount(), 2);
        EXPECT_LE(distance(first_point(stroke), LineStart), max_offset + RoundingTolerance);
        EXPECT_LE(distance(last_point(stroke), LineEnd), max_offset + RoundingTolerance);
        for (int i = 0; i < stroke.elementCount(); ++i)
            EXPECT_LE(distance_to_segment(element_point(stroke, i), LineStart, LineEnd), max_offset + RoundingTolerance);
    }
}

TEST(RoughGeometryEngine, DifferentSeedsGiveDifferentLineOutlines)
{
    const QPainterPath first = rough_line_outline(1.0, 1).at(0);
    const QPainterPath second = rough_line_outline(1.0, 2).at(0);

    bool differs = first.elementCount() != second.elementCount();
    for (int i = 0; !differs && i < first.elementCount(); ++i)
        differs = distance(element_point(first, i), element_point(second, i)) > 1e-9;
    EXPECT_TRUE(differs);
}

TEST(RoughGeometryEngine, ZeroRoughnessKeepsTheLineGeometry)
{
    const double tolerance = 0.01;
    const std::vector<QPainterPath> strokes = rough_line_outline(0.0, 42);

    ASSERT_EQ(2u, strokes.size());
    for (const QPainterPath &stroke : strokes) {
        ASSERT_GE(stroke.elementCount(), 2);
        EXPECT_LE(distance(first_point(stroke), LineStart), tolerance);
        EXPECT_LE(distance(last_point(stroke), LineEnd), tolerance);
        for (int i = 0; i < stroke.elementCount(); ++i)
            EXPECT_LE(distance_to_segment(element_point(stroke, i), LineStart, LineEnd), tolerance);
    }
}

namespace {

struct NamedShape {
    const char *name;
    QPainterPath path;
};

void PrintTo(const NamedShape &shape, std::ostream *out) { *out << shape.name; }

QPainterPath rectangle_path()
{
    QPainterPath path;
    path.addRect(10, 10, 100, 50);
    return path;
}

QPainterPath ellipse_path()
{
    QPainterPath path;
    path.addEllipse(0, 0, 80, 40);
    return path;
}

QPainterPath triangle_path()
{
    QPainterPath path;
    path.addPolygon(QPolygonF({QPointF(0, 0), QPointF(100, 0), QPointF(50, 80), QPointF(0, 0)}));
    path.closeSubpath();
    return path;
}

QPainterPath rounded_rect_path()
{
    QPainterPath path;
    path.addRoundedRect(0, 0, 100, 60, 12, 12);
    return path;
}

QPainterPath closed_cubic_path()
{
    QPainterPath path(QPointF(0, 0));
    path.cubicTo(QPointF(40, -40), QPointF(80, 40), QPointF(100, 0));
    path.cubicTo(QPointF(80, 60), QPointF(20, 60), QPointF(0, 0));
    path.closeSubpath();
    return path;
}

class ClosedShapeOutline : public ::testing::TestWithParam<NamedShape> {};

std::string shape_name(const ::testing::TestParamInfo<NamedShape> &info) { return info.param.name; }

}  // namespace

TEST_P(ClosedShapeOutline, StrokesLookClosedAndStayNearTheShape)
{
    const double max_offset = 5.0;
    const QPainterPath &shape = GetParam().path;
    const QRectF allowed_area = shape.boundingRect().adjusted(-max_offset, -max_offset, max_offset, max_offset);

    for (std::uint32_t seed : {1u, 42u, 1234u}) {
        const std::vector<QPainterPath> strokes = hpq::RoughGeometryEngine().outline(shape, rough_options(1.0), seed);

        ASSERT_EQ(2u, strokes.size());
        for (const QPainterPath &stroke : strokes) {
            ASSERT_GE(stroke.elementCount(), 2);
            EXPECT_LE(distance(first_point(stroke), last_point(stroke)), max_offset);
            const QRectF bounds = stroke.boundingRect();
            EXPECT_TRUE(allowed_area.contains(bounds))
                << "seed " << seed << ": stroke bounds (" << bounds.left() << ", " << bounds.top() << ", "
                << bounds.right() << ", " << bounds.bottom() << ")";
        }
    }
}

INSTANTIATE_TEST_SUITE_P(RoughGeometryEngine, ClosedShapeOutline,
                         ::testing::Values(NamedShape{"Rectangle", rectangle_path()},
                                           NamedShape{"Ellipse", ellipse_path()},
                                           NamedShape{"Triangle", triangle_path()},
                                           NamedShape{"RoundedRect", rounded_rect_path()},
                                           NamedShape{"ClosedCubic", closed_cubic_path()}),
                         shape_name);

namespace {

const QPointF OpenCubicStart(0, 0);
const QPointF OpenCubicEnd(100, 0);

QPainterPath open_cubic_path()
{
    QPainterPath path(OpenCubicStart);
    path.cubicTo(QPointF(30, -40), QPointF(70, 40), OpenCubicEnd);
    return path;
}

}  // namespace

TEST(RoughGeometryEngine, OpenCubicOutlineStaysOpen)
{
    const double max_offset = 5.0;
    const QPointF start = OpenCubicStart;
    const QPointF end = OpenCubicEnd;
    const QPainterPath curve = open_cubic_path();

    for (std::uint32_t seed : {1u, 42u, 1234u}) {
        const std::vector<QPainterPath> strokes = hpq::RoughGeometryEngine().outline(curve, rough_options(1.0), seed);

        ASSERT_EQ(2u, strokes.size());
        for (const QPainterPath &stroke : strokes) {
            ASSERT_GE(stroke.elementCount(), 2);
            EXPECT_LE(distance(first_point(stroke), start), max_offset + RoundingTolerance);
            EXPECT_LE(distance(last_point(stroke), end), max_offset + RoundingTolerance);
            EXPECT_GT(distance(first_point(stroke), last_point(stroke)), max_offset);
        }
    }
}

namespace {

constexpr double DegreesPerRadian = 180.0 / 3.14159265358979323846;

// Largest change of direction, in degrees, between two consecutive segments of a polyline.
double max_turning_angle(const QPolygonF &polyline)
{
    double max_angle = 0.0;
    QPointF previous_direction;
    for (int i = 1; i < polyline.size(); ++i) {
        const QPointF direction = polyline[i] - polyline[i - 1];
        if (std::hypot(direction.x(), direction.y()) < 1e-6)
            continue;
        if (!previous_direction.isNull()) {
            const double cross = previous_direction.x() * direction.y() - previous_direction.y() * direction.x();
            const double angle = std::abs(std::atan2(cross, QPointF::dotProduct(previous_direction, direction)));
            max_angle = std::max(max_angle, angle * DegreesPerRadian);
        }
        previous_direction = direction;
    }
    return max_angle;
}

double max_turning_angle(const QPainterPath &path)
{
    double max_angle = 0.0;
    for (const QPolygonF &polyline : path.toSubpathPolygons())
        max_angle = std::max(max_angle, max_turning_angle(polyline));
    return max_angle;
}

class SmoothOutline : public ::testing::TestWithParam<NamedShape> {};

}  // namespace

// A hand-drawn stroke wobbles smoothly around the shape: it never kinks much more
// than the shape itself does, so it has no zigzag, hook or self-loop.
TEST_P(SmoothOutline, StrokesTurnAlmostAsGentlyAsTheShape)
{
    const double allowed_extra_turn = 20.0;
    const QPainterPath &shape = GetParam().path;
    const double shape_turn = max_turning_angle(shape);

    for (std::uint32_t seed : {1u, 42u, 1234u, 12345u}) {
        const std::vector<QPainterPath> strokes = hpq::RoughGeometryEngine().outline(shape, rough_options(1.0), seed);

        for (const QPainterPath &stroke : strokes)
            EXPECT_LE(max_turning_angle(stroke), shape_turn + allowed_extra_turn) << "seed " << seed;
    }
}

INSTANTIATE_TEST_SUITE_P(RoughGeometryEngine, SmoothOutline,
                         ::testing::Values(NamedShape{"Line", line_path(LineStart, LineEnd)},
                                           NamedShape{"Rectangle", rectangle_path()},
                                           NamedShape{"Ellipse", ellipse_path()},
                                           NamedShape{"Triangle", triangle_path()},
                                           NamedShape{"RoundedRect", rounded_rect_path()},
                                           NamedShape{"ClosedCubic", closed_cubic_path()},
                                           NamedShape{"OpenCubic", open_cubic_path()}),
                         shape_name);
