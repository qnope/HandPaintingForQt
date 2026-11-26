#include "StyleRendering.h"

#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QPalette>
#include <QPolygonF>
#include <QSpinBox>

#include <algorithm>
#include <vector>

using hpq::test::DrawCall;

namespace {

enum class Direction { Up, Down };

// The distinct corners, in device coordinates, of the only closed polygon of the call,
// or nothing when the path is not exactly one closed polygon.
QPolygonF closed_polygon_corners(const DrawCall &call) {
    const QList<QPolygonF> polygons = call.path.toSubpathPolygons();
    if (polygons.size() != 1 || !polygons.front().isClosed()) {
        return {};
    }
    QPolygonF corners = polygons.front();
    corners.removeLast();
    return call.transform.map(corners);
}

// A filled triangle in the ButtonText color whose tip is the only highest (Up) or lowest (Down) corner.
bool is_arrow(const DrawCall &call, Direction direction, const QPalette &palette) {
    if (!call.is_fill() || call.brush.style() != Qt::SolidPattern ||
        call.brush.color() != palette.color(QPalette::ButtonText)) {
        return false;
    }
    std::vector<QPointF> corners;
    const QPolygonF polygon = closed_polygon_corners(call);
    corners.assign(polygon.begin(), polygon.end());
    if (corners.size() != 3) {
        return false;
    }
    const auto is_before = [direction](QPointF first, QPointF second) {
        return direction == Direction::Up ? first.y() < second.y() : first.y() > second.y();
    };
    std::sort(corners.begin(), corners.end(), is_before);
    return is_before(corners[0], corners[1]);
}

bool is_dark_outline(const DrawCall &call, const QPalette &palette) {
    return call.kind == DrawCall::Kind::Path && call.pen.style() != Qt::NoPen &&
           call.pen.color() == palette.color(QPalette::Dark);
}

void expect_flat_frame_and_arrows(QAbstractSpinBox &spin_box) {
    spin_box.resize(120, 30);
    const QPalette palette = hpq::sketch_theme().palette;

    const std::vector<DrawCall> calls = hpq::test::render_with_style(spin_box);

    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_EQ(std::count_if(calls.begin(), calls.end(),
                            [&palette](const DrawCall &call) { return is_dark_outline(call, palette); }),
              1);
    EXPECT_EQ(std::count_if(calls.begin(), calls.end(),
                            [&palette](const DrawCall &call) { return is_arrow(call, Direction::Up, palette); }),
              1);
    EXPECT_EQ(std::count_if(calls.begin(), calls.end(),
                            [&palette](const DrawCall &call) { return is_arrow(call, Direction::Down, palette); }),
              1);
}

}  // namespace

TEST(SpinBoxTest, SpinBoxDrawsDarkFrameAndUpAndDownArrowsWithoutImage) {
    QSpinBox spin_box;
    expect_flat_frame_and_arrows(spin_box);
}

TEST(SpinBoxTest, DoubleSpinBoxDrawsDarkFrameAndUpAndDownArrowsWithoutImage) {
    QDoubleSpinBox spin_box;
    expect_flat_frame_and_arrows(spin_box);
}
