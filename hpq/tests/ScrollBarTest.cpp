#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPalette>
#include <QPolygonF>
#include <QScrollBar>

#include <cmath>
#include <map>
#include <optional>
#include <vector>

using hpq::test::DrawCall;

namespace {

constexpr int ScrollBarLength = 180;
constexpr int ScrollBarThickness = 16;

enum class Direction { Up, Down, Left, Right };

bool is_same(qreal first, qreal second) { return std::abs(first - second) < 1e-6; }

// The 3 corners, in device coordinates, of a call that fills one closed triangle in the
// ButtonText color, or nothing.
std::optional<QPolygonF> arrow_triangle(const DrawCall &call, const QPalette &palette) {
    if (!call.is_fill() || call.brush.style() != Qt::SolidPattern ||
        call.brush.color() != palette.color(QPalette::Active, QPalette::ButtonText)) {
        return std::nullopt;
    }
    const QList<QPolygonF> polygons = call.path.toSubpathPolygons();
    if (polygons.size() != 1 || !polygons.front().isClosed() || polygons.front().size() != 4) {
        return std::nullopt;
    }
    QPolygonF corners = polygons.front();
    corners.removeLast();
    return call.transform.map(corners);
}

// The side the tip points to: the 2 other corners form a base parallel to that side.
std::optional<Direction> direction(const QPolygonF &triangle) {
    for (int tip = 0; tip < 3; ++tip) {
        const QPointF &first = triangle[(tip + 1) % 3];
        const QPointF &second = triangle[(tip + 2) % 3];
        const QPointF &point = triangle[tip];
        if (is_same(first.y(), second.y()) && !is_same(point.y(), first.y())) {
            return point.y() < first.y() ? Direction::Up : Direction::Down;
        }
        if (is_same(first.x(), second.x()) && !is_same(point.x(), first.x())) {
            return point.x() < first.x() ? Direction::Left : Direction::Right;
        }
    }
    return std::nullopt;
}

std::map<Direction, int> arrow_counts(const std::vector<DrawCall> &calls, const QPalette &palette) {
    std::map<Direction, int> counts;
    for (const DrawCall &call : calls) {
        if (const auto triangle = arrow_triangle(call, palette)) {
            if (const auto side = direction(*triangle)) {
                ++counts[*side];
            }
        }
    }
    return counts;
}

std::map<Direction, int> render_scroll_bar(Qt::Orientation orientation, bool &drew_image) {
    QScrollBar scroll_bar(orientation);
    scroll_bar.setRange(0, 100);
    if (orientation == Qt::Horizontal) {
        scroll_bar.resize(ScrollBarLength, ScrollBarThickness);
    } else {
        scroll_bar.resize(ScrollBarThickness, ScrollBarLength);
    }
    const std::vector<DrawCall> calls = hpq::test::render_with_style(scroll_bar);
    drew_image = hpq::test::has_image(calls);
    return arrow_counts(calls, hpq::sketch_theme().palette());
}

}  // namespace

TEST(ScrollBarTest, HorizontalScrollBarDrawsOneLeftAndOneRightVectorArrow) {
    bool drew_image = true;
    const std::map<Direction, int> counts = render_scroll_bar(Qt::Horizontal, drew_image);

    EXPECT_FALSE(drew_image);
    EXPECT_EQ(counts, (std::map<Direction, int>{{Direction::Left, 1}, {Direction::Right, 1}}));
}

TEST(ScrollBarTest, VerticalScrollBarDrawsOneUpAndOneDownVectorArrow) {
    bool drew_image = true;
    const std::map<Direction, int> counts = render_scroll_bar(Qt::Vertical, drew_image);

    EXPECT_FALSE(drew_image);
    EXPECT_EQ(counts, (std::map<Direction, int>{{Direction::Up, 1}, {Direction::Down, 1}}));
}
