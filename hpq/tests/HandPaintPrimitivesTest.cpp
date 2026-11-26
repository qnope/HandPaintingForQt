#include "ExactTheme.h"
#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>
#include <QPolygonF>

#include <cmath>
#include <functional>

#include <cstdint>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QPolygonF Triangle({QPointF(10, 10), QPointF(90, 20), QPointF(40, 70)});
const QRectF TriangleBounds(10, 10, 80, 60);

// The target receives the outline of each shape as one stroke, which is the shape itself: the
// theme draws without the wobble of the hand.
class HandPaintPrimitivesTest : public testing::Test {
protected:
    // Draws with `draw` on a HandPaintDevice of `theme` whose target is m_target, with a pen and
    // a brush set.
    void draw(const std::function<void(QPainter &)> &draw, const hpq::Theme &theme) {
        const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
        QPainter target_painter(&m_target);
        hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
        QPainter painter(&device);
        painter.setPen(QPen(Qt::red, 2.0));
        painter.setBrush(QBrush(Qt::blue));
        draw(painter);
        painter.end();
    }

    void draw(const std::function<void(QPainter &)> &draw) { this->draw(draw, m_theme); }

    // The shapes whose outline the target received.
    std::vector<QPainterPath> outlined_shapes() const {
        std::vector<QPainterPath> shapes;
        for (const DrawCall &stroke : m_target.engine().strokes()) {
            shapes.push_back(stroke.path);
        }
        return shapes;
    }

    // Draws with `draw` and checks that the outline of one shape was drawn, whose bounding box is `bounds`.
    void expect_one_shape_with_bounds(const std::function<void(QPainter &)> &draw_primitive, const QRectF &bounds) {
        draw(draw_primitive);
        ASSERT_EQ(outlined_shapes().size(), 1u);
        EXPECT_EQ(outlined_shapes()[0].boundingRect(), bounds);
    }

    hpq::Theme m_theme = hpq::test::exact(hpq::sketch_theme());
    RecordingPaintDevice m_target;
};

TEST_F(HandPaintPrimitivesTest, LineIsDrawnAsOneShape) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawLine(QPointF(10, 20), QPointF(90, 60)); },
                                 QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, IntegerLineIsDrawnAsOneShape) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) {
            const QLine line(10, 20, 90, 60);
            painter.paintEngine()->drawLines(&line, 1);
        },
        QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, RectangleIsDrawnAsOneShape) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawRect(QRectF(10, 20, 80, 40)); },
                                 QRectF(10, 20, 80, 40));
}

// How many points of `strokes` are inside `rectangle`, away from its edge by more than a rounding.
int points_inside(const std::vector<QPainterPath> &strokes, const QRectF &rectangle) {
    const QRectF inside = rectangle.adjusted(0.01, 0.01, -0.01, -0.01);
    int count = 0;
    for (const QPainterPath &stroke : strokes) {
        for (int index = 0; index < stroke.elementCount(); ++index) {
            count += inside.contains(stroke.elementAt(index)) ? 1 : 0;
        }
    }
    return count;
}

// The hand wobbles: the outline of a filled rectangle goes in and out of it.
TEST_F(HandPaintPrimitivesTest, OutlineOfAFilledRectangleGoesInsideIt) {
    const QRectF rectangle(10, 20, 80, 40);

    draw([&](QPainter &painter) { painter.drawRect(rectangle); }, hpq::sketch_theme());

    EXPECT_GT(points_inside(outlined_shapes(), rectangle), 0);
}

// A rectangle that fills nothing frames what is inside it: its outline is kept out of it.
TEST_F(HandPaintPrimitivesTest, UnfilledRectangleIsAFrame) {
    const QRectF frame(10, 20, 80, 40);

    draw(
        [&](QPainter &painter) {
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(frame);
        },
        hpq::sketch_theme());

    ASSERT_FALSE(outlined_shapes().empty());
    EXPECT_EQ(points_inside(outlined_shapes(), frame), 0);
}

TEST_F(HandPaintPrimitivesTest, EllipseIsDrawnAsOneShape) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawEllipse(QRectF(10, 20, 80, 40)); },
                                 QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, IntegerEllipseIsDrawnAsOneShape) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) { painter.paintEngine()->drawEllipse(QRect(10, 20, 80, 40)); }, QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, OddEvenPolygonKeepsItsFillRule) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolygon(Triangle, Qt::OddEvenFill); },
                                 TriangleBounds);
    EXPECT_EQ(outlined_shapes()[0].fillRule(), Qt::OddEvenFill);
}

TEST_F(HandPaintPrimitivesTest, WindingPolygonKeepsItsFillRule) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolygon(Triangle, Qt::WindingFill); },
                                 TriangleBounds);
    EXPECT_EQ(outlined_shapes()[0].fillRule(), Qt::WindingFill);
}

TEST_F(HandPaintPrimitivesTest, ConvexPolygonIsDrawnAsOneShape) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawConvexPolygon(Triangle); }, TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, IntegerPolygonIsDrawnAsOneShape) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) {
            const QPoint points[] = {QPoint(10, 10), QPoint(90, 20), QPoint(40, 70)};
            painter.paintEngine()->drawPolygon(points, 3, QPaintEngine::OddEvenMode);
        },
        TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, PolylineIsDrawnAsOneShape) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolyline(Triangle); }, TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, PathIsDrawnAsOneShape) {
    QPainterPath path;
    path.moveTo(10, 10);
    path.cubicTo(30, 0, 60, 80, 90, 70);
    path.closeSubpath();
    expect_one_shape_with_bounds([&path](QPainter &painter) { painter.drawPath(path); }, path.boundingRect());
}

TEST_F(HandPaintPrimitivesTest, PolylineIsAnOpenShapeAndIsNotFilled) {
    draw([](QPainter &painter) { painter.drawPolyline(Triangle); });

    ASSERT_EQ(outlined_shapes().size(), 1u);
    const QPainterPath shape = outlined_shapes()[0];
    ASSERT_EQ(shape.elementCount(), Triangle.size());
    EXPECT_NE(QPointF(shape.elementAt(shape.elementCount() - 1)), Triangle.first());
    EXPECT_TRUE(m_target.engine().fills().empty());
}

// Five-point star drawn in one stroke: its center is inside with WindingFill, outside with OddEvenFill.
QPolygonF star(const QPointF &center, double radius) {
    const double pi = std::acos(-1.0);
    QPolygonF points;
    for (int index = 0; index < 5; ++index) {
        const double angle = -pi / 2 + index * 4 * pi / 5;
        points << center + radius * QPointF(std::cos(angle), std::sin(angle));
    }
    return points;
}

// Draws the star with `fill_rule` through a rough HandPaintDevice on a white image and gives the
// color at the star center.
QColor rough_star_center_color(Qt::FillRule fill_rule) {
    const QPointF center(100, 100);
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter target_painter(&image);
    const hpq::Theme theme = hpq::sketch_theme();
    const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black, 1.0));
    painter.setBrush(QBrush(Qt::red));
    painter.drawPolygon(star(center, 90), fill_rule);
    painter.end();
    target_painter.end();
    return image.pixelColor(center.toPoint());
}

TEST(HandPaintPrimitivesRenderingTest, WindingStarIsFilledAtItsCenter) {
    EXPECT_EQ(rough_star_center_color(Qt::WindingFill), QColor(Qt::red));
}

TEST(HandPaintPrimitivesRenderingTest, OddEvenStarHasAHoleAtItsCenter) {
    EXPECT_EQ(rough_star_center_color(Qt::OddEvenFill), QColor(Qt::white));
}

}  // namespace
