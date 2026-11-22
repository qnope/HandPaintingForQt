#include "RecordingPaintDevice.h"

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>
#include <QPolygonF>

#include <cmath>
#include <functional>

namespace {

using hpq::test::RecordingPaintDevice;

// Geometry engine that records the shapes it is given and gives them back unchanged.
class SpyGeometryEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &,
                                      std::uint32_t) const override {
        m_outline_shapes.push_back(shape);
        return {shape};
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        return shape;
    }

    const std::vector<QPainterPath> &outline_shapes() const { return m_outline_shapes; }

private:
    mutable std::vector<QPainterPath> m_outline_shapes;
};

const QPolygonF Triangle({QPointF(10, 10), QPointF(90, 20), QPointF(40, 70)});
const QRectF TriangleBounds(10, 10, 80, 60);

class HandPaintPrimitivesTest : public testing::Test {
protected:
    // Draws with `draw` on a HandPaintDevice whose target is m_target, with a pen and a brush set.
    void draw(const std::function<void(QPainter &)> &draw) {
        QPainter target_painter(&m_target);
        hpq::HandPaintDevice device(target_painter, m_theme, m_spy);
        QPainter painter(&device);
        painter.setPen(QPen(Qt::red, 2.0));
        painter.setBrush(QBrush(Qt::blue));
        draw(painter);
        painter.end();
    }

    // Draws with `draw` and checks that the spy saw one shape whose bounding box is `bounds`.
    void expect_one_shape_with_bounds(const std::function<void(QPainter &)> &draw_primitive, const QRectF &bounds) {
        draw(draw_primitive);
        ASSERT_EQ(m_spy.outline_shapes().size(), 1u);
        EXPECT_EQ(m_spy.outline_shapes()[0].boundingRect(), bounds);
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    SpyGeometryEngine m_spy;
    RecordingPaintDevice m_target;
};

TEST_F(HandPaintPrimitivesTest, LineGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawLine(QPointF(10, 20), QPointF(90, 60)); },
                                 QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, IntegerLineGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) {
            const QLine line(10, 20, 90, 60);
            painter.paintEngine()->drawLines(&line, 1);
        },
        QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, RectangleGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawRect(QRectF(10, 20, 80, 40)); },
                                 QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, EllipseGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawEllipse(QRectF(10, 20, 80, 40)); },
                                 QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, IntegerEllipseGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) { painter.paintEngine()->drawEllipse(QRect(10, 20, 80, 40)); }, QRectF(10, 20, 80, 40));
}

TEST_F(HandPaintPrimitivesTest, OddEvenPolygonGoesThroughGeometryEngineWithItsFillRule) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolygon(Triangle, Qt::OddEvenFill); },
                                 TriangleBounds);
    EXPECT_EQ(m_spy.outline_shapes()[0].fillRule(), Qt::OddEvenFill);
}

TEST_F(HandPaintPrimitivesTest, WindingPolygonGoesThroughGeometryEngineWithItsFillRule) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolygon(Triangle, Qt::WindingFill); },
                                 TriangleBounds);
    EXPECT_EQ(m_spy.outline_shapes()[0].fillRule(), Qt::WindingFill);
}

TEST_F(HandPaintPrimitivesTest, ConvexPolygonGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawConvexPolygon(Triangle); }, TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, IntegerPolygonGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds(
        [](QPainter &painter) {
            const QPoint points[] = {QPoint(10, 10), QPoint(90, 20), QPoint(40, 70)};
            painter.paintEngine()->drawPolygon(points, 3, QPaintEngine::OddEvenMode);
        },
        TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, PolylineGoesThroughGeometryEngine) {
    expect_one_shape_with_bounds([](QPainter &painter) { painter.drawPolyline(Triangle); }, TriangleBounds);
}

TEST_F(HandPaintPrimitivesTest, PathGoesThroughGeometryEngine) {
    QPainterPath path;
    path.moveTo(10, 10);
    path.cubicTo(30, 0, 60, 80, 90, 70);
    path.closeSubpath();
    expect_one_shape_with_bounds([&path](QPainter &painter) { painter.drawPath(path); }, path.boundingRect());
}

TEST_F(HandPaintPrimitivesTest, PolylineIsAnOpenShapeAndIsNotFilled) {
    draw([](QPainter &painter) { painter.drawPolyline(Triangle); });

    ASSERT_EQ(m_spy.outline_shapes().size(), 1u);
    const QPainterPath &shape = m_spy.outline_shapes()[0];
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
    const hpq::RoughGeometryEngine geometry_engine;
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
