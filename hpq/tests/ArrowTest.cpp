#include "RecordingPaintDevice.h"
#include "StyleRendering.h"

#include <hpq/HandPaintStyle.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPainter>
#include <QPolygonF>
#include <QRect>
#include <QStyleOption>

#include <algorithm>
#include <functional>
#include <vector>

namespace {

using hpq::test::DrawCall;

const QRect ArrowRect(10, 10, 20, 20);

std::vector<DrawCall> draw_arrow(QStyle::PrimitiveElement arrow) {
    hpq::HandPaintStyle style;
    QStyleOption option;
    option.rect = ArrowRect;
    option.palette = hpq::SketchTheme().palette();
    option.state = QStyle::State_Enabled;

    hpq::test::RecordingPaintDevice device;
    {
        QPainter painter(&device);
        style.drawPrimitive(arrow, &option, &painter);
    }
    return device.engine().calls();
}

// The distinct corners of the only closed polygon of the call, or nothing when the path
// is not exactly one closed polygon.
QPolygonF closed_polygon_corners(const DrawCall &call) {
    const QList<QPolygonF> polygons = call.path.toSubpathPolygons();
    if (polygons.size() != 1 || !polygons.front().isClosed()) {
        return {};
    }
    QPolygonF corners = polygons.front();
    corners.removeLast();
    return corners;
}

QPolygonF arrow_triangle(QStyle::PrimitiveElement arrow) {
    const std::vector<DrawCall> calls = draw_arrow(arrow);
    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_EQ(calls.size(), 1u);
    if (calls.size() != 1) {
        return {};
    }
    const DrawCall &call = calls.front();
    EXPECT_TRUE(call.is_fill());
    EXPECT_EQ(call.brush.style(), Qt::SolidPattern);
    EXPECT_EQ(call.brush.color(), hpq::SketchTheme().palette().color(QPalette::ButtonText));
    return call.transform.map(closed_polygon_corners(call));
}

void expect_triangle_inside_rect(const QPolygonF &triangle) {
    ASSERT_EQ(triangle.size(), 3);
    for (const QPointF &corner : triangle) {
        EXPECT_TRUE(QRectF(ArrowRect).contains(corner)) << corner.x() << ", " << corner.y();
    }
}

// The tip is the only corner that has the best value of the key.
void expect_tip(const QPolygonF &triangle, const std::function<bool(QPointF, QPointF)> &is_before) {
    ASSERT_EQ(triangle.size(), 3);
    std::vector<QPointF> corners(triangle.begin(), triangle.end());
    std::sort(corners.begin(), corners.end(), is_before);
    EXPECT_TRUE(is_before(corners[0], corners[1]));
}

bool is_higher(QPointF first, QPointF second) { return first.y() < second.y(); }
bool is_lower(QPointF first, QPointF second) { return first.y() > second.y(); }
bool is_more_left(QPointF first, QPointF second) { return first.x() < second.x(); }
bool is_more_right(QPointF first, QPointF second) { return first.x() > second.x(); }

}  // namespace

TEST(ArrowTest, UpArrowIsOneFilledTrianglePointingUp) {
    const QPolygonF triangle = arrow_triangle(QStyle::PE_IndicatorArrowUp);
    expect_triangle_inside_rect(triangle);
    expect_tip(triangle, is_higher);
}

TEST(ArrowTest, DownArrowIsOneFilledTrianglePointingDown) {
    const QPolygonF triangle = arrow_triangle(QStyle::PE_IndicatorArrowDown);
    expect_triangle_inside_rect(triangle);
    expect_tip(triangle, is_lower);
}

TEST(ArrowTest, LeftArrowIsOneFilledTrianglePointingLeft) {
    const QPolygonF triangle = arrow_triangle(QStyle::PE_IndicatorArrowLeft);
    expect_triangle_inside_rect(triangle);
    expect_tip(triangle, is_more_left);
}

TEST(ArrowTest, RightArrowIsOneFilledTrianglePointingRight) {
    const QPolygonF triangle = arrow_triangle(QStyle::PE_IndicatorArrowRight);
    expect_triangle_inside_rect(triangle);
    expect_tip(triangle, is_more_right);
}
