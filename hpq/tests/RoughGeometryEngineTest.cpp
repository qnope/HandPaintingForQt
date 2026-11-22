#include <hpq/RoughGeometryEngine.h>

#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
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

}  // namespace

TEST(RoughGeometryEngine, LineOutlineHasStrokeCountStrokesNearTheSegment)
{
    const double max_offset = 5.0;
    const std::vector<QPainterPath> strokes = rough_line_outline(1.0, 42);

    ASSERT_EQ(2u, strokes.size());
    for (const QPainterPath &stroke : strokes) {
        ASSERT_GE(stroke.elementCount(), 2);
        EXPECT_LE(distance(first_point(stroke), LineStart), max_offset);
        EXPECT_LE(distance(last_point(stroke), LineEnd), max_offset);
        for (int i = 0; i < stroke.elementCount(); ++i)
            EXPECT_LE(distance_to_segment(element_point(stroke, i), LineStart, LineEnd), max_offset);
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
