#include "StyleRendering.h"

#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QGradient>
#include <QPalette>
#include <QPolygonF>
#include <QScrollBar>

#include <algorithm>
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
    return arrow_counts(calls, hpq::sketch_theme().palette);
}

// A color no theme has but for its buttons.
const QColor ButtonGreen(0x20, 0xe0, 0x20);

bool is_green(const QColor &color) {
    return color.green() > std::max(color.red(), color.blue()) + 40;
}

// True when the pen or the brush of `call` draws in a green: a plain color, or a stop of a gradient.
bool draws_in_green(const DrawCall &call) {
    if (call.kind != DrawCall::Kind::Path) {
        return false;
    }
    if (call.pen.style() != Qt::NoPen && is_green(call.pen.color())) {
        return true;
    }
    if (const QGradient *gradient = call.brush.gradient()) {
        const QGradientStops stops = gradient->stops();
        return std::any_of(stops.begin(), stops.end(),
                           [](const QGradientStop &stop) { return is_green(stop.second); });
    }
    return call.brush.style() != Qt::NoBrush && is_green(call.brush.color());
}

}  // namespace

// The groove and the handle are not buttons: only the 2 line buttons take the Button color.
TEST(ScrollBarTest, OnlyTheLineButtonsAreInTheButtonColor) {
    QPalette palette = hpq::sketch_theme().palette;
    palette.setColor(QPalette::Button, ButtonGreen);
    for (const Qt::Orientation orientation : {Qt::Horizontal, Qt::Vertical}) {
        QScrollBar scroll_bar(orientation);
        scroll_bar.setRange(0, 100);
        if (orientation == Qt::Horizontal) {
            scroll_bar.resize(ScrollBarLength, ScrollBarThickness);
        } else {
            scroll_bar.resize(ScrollBarThickness, ScrollBarLength);
        }

        const std::vector<DrawCall> calls = hpq::test::render_with_style(scroll_bar, palette);

        int line_button_count = 0;
        for (const DrawCall &call : calls) {
            if (!draws_in_green(call)) {
                continue;
            }
            EXPECT_EQ(call.brush, QBrush(ButtonGreen)) << "orientation " << orientation;
            ++line_button_count;
        }
        EXPECT_EQ(line_button_count, 2) << "orientation " << orientation;
    }
}

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
