#include "RecordingPaintDevice.h"

#include <hpq/ChinaInkTheme.h>
#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>

#include <gtest/gtest.h>

#include <QPainter>

#include <functional>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QColor PenColor(0x20, 0x40, 0x80);
const QColor FillColor(0xc0, 0x40, 0x40);
constexpr double PenWidth = 2.0;
const QLineF Line(QPointF(20, 100), QPointF(180, 100));
const QRectF Rectangle(40, 40, 100, 60);
// A brush line is wider than its pen, and wider still where the hand presses: no more than this.
constexpr double WidestBrushLine = 8.0;

// Geometry engine that returns the shape unchanged.
class IdentityGeometryEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &,
                                      std::uint32_t) const override {
        return {shape};
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        return shape;
    }

    QPainterPath hachure(const QPainterPath &, const hpq::RoughOptions &, std::uint32_t) const override {
        return QPainterPath();
    }
};

// Geometry engine that hatches every shape with its two diagonals.
class DiagonalHachureEngine : public IdentityGeometryEngine {
public:
    QPainterPath hachure(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        const QRectF bounds = shape.boundingRect();
        QPainterPath lines(bounds.topLeft());
        lines.lineTo(bounds.bottomRight());
        lines.moveTo(bounds.topRight());
        lines.lineTo(bounds.bottomLeft());
        return lines;
    }
};

// Draws with `draw` on a HandPaintDevice of `theme` and `geometry_engine`, in its own painting
// session, and returns the calls the target received.
std::vector<DrawCall> calls_of(const hpq::Theme &theme, const hpq::GeometryEngine &geometry_engine,
                               const std::function<void(QPainter &)> &draw) {
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    draw(painter);
    painter.end();
    return target.engine().calls();
}

std::vector<DrawCall> ink_calls(const std::function<void(QPainter &)> &draw) {
    return calls_of(hpq::china_ink_theme(), IdentityGeometryEngine(), draw);
}

void draw_line(QPainter &painter) {
    painter.setPen(QPen(PenColor, PenWidth));
    painter.drawLine(Line);
}

// Draws Rectangle without outline, filled with FillColor.
void fill_rectangle(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(FillColor);
    painter.drawRect(Rectangle);
}

// The height of the part of `area` whose x is between `left` and `right`.
double height_between(const QPainterPath &area, double left, double right) {
    QPainterPath band;
    band.addRect(QRectF(QPointF(left, 0), QPointF(right, 200)));
    return area.intersected(band).boundingRect().height();
}

}  // namespace

TEST(InkMedium, LineIsAnAreaFilledWithThePlainPenColor) {
    const std::vector<DrawCall> calls = ink_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, QBrush(PenColor));
}

TEST(InkMedium, AreaOfALineFollowsItFromEndToEnd) {
    const std::vector<DrawCall> calls = ink_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    const QRectF bounds = calls[0].path.boundingRect();
    EXPECT_LE(bounds.left(), Line.x1());
    EXPECT_GE(bounds.right(), Line.x2());
    EXPECT_GT(bounds.height(), PenWidth / 2.0);
    EXPECT_LT(bounds.height(), WidestBrushLine);
    EXPECT_LT(bounds.width(), Line.length() + WidestBrushLine);
    EXPECT_NEAR(bounds.center().y(), Line.y1(), WidestBrushLine / 2.0);
}

// The brush lands at the start of a line and lifts at its end.
TEST(InkMedium, LineStartsWideAndEndsInAPoint) {
    const std::vector<DrawCall> calls = ink_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    const double start_height = height_between(calls[0].path, Line.x1(), Line.x1() + 6.0);
    const double end_height = height_between(calls[0].path, Line.x2() - 3.0, Line.x2() + WidestBrushLine);
    EXPECT_GT(start_height, PenWidth);
    EXPECT_LT(end_height, PenWidth / 2.0);
}

TEST(InkMedium, SameLineGivesTheSameArea) {
    const std::vector<DrawCall> first = ink_calls(draw_line);
    const std::vector<DrawCall> second = ink_calls(draw_line);

    ASSERT_EQ(first.size(), 1u);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_EQ(first[0].path, second[0].path);
}

TEST(InkMedium, PressureDiffersFromOneLineToAnother) {
    const QPointF offset(0, 40);
    const std::vector<DrawCall> first = ink_calls(draw_line);
    const std::vector<DrawCall> second = ink_calls([&](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth));
        painter.drawLine(Line.translated(offset));
    });

    ASSERT_EQ(first.size(), 1u);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_NE(first[0].path.translated(offset), second[0].path);
}

TEST(InkMedium, OutlineOfARectangleIsOneFilledArea) {
    const std::vector<DrawCall> calls = ink_calls([](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth));
        painter.drawRect(Rectangle);
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, QBrush(PenColor));
    EXPECT_TRUE(Rectangle.adjusted(-WidestBrushLine, -WidestBrushLine, WidestBrushLine, WidestBrushLine)
                    .contains(calls[0].path.boundingRect()));
}

TEST(InkMedium, DashedLineIsDrawnByItsPen) {
    const QPen dashed(PenColor, PenWidth, Qt::DashLine);
    const std::vector<DrawCall> calls = ink_calls([&](QPainter &painter) {
        painter.setPen(dashed);
        painter.drawLine(Line);
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_stroke());
    EXPECT_EQ(calls[0].pen, dashed);
}

TEST(InkMedium, FillKeepsThePlainBrush) {
    const std::vector<DrawCall> calls = ink_calls(fill_rectangle);

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, QBrush(FillColor));
}

TEST(InkMedium, TextKeepsThePlainPen) {
    const std::vector<DrawCall> calls = ink_calls([](QPainter &painter) {
        painter.setPen(PenColor);
        painter.drawText(10, 30, "Hello");
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].kind, DrawCall::Kind::Text);
    EXPECT_EQ(calls[0].pen.brush(), QBrush(PenColor));
}

TEST(InkMedium, HatchedFillIsThePaperUnderBrushLines) {
    const hpq::Theme theme = hpq::china_ink_theme(hpq::FillStyle::Hachure);

    const std::vector<DrawCall> calls = calls_of(theme, DiagonalHachureEngine(), fill_rectangle);

    ASSERT_EQ(calls.size(), 2u);
    // The paper under the hachures.
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, theme.palette().window());
    // The hachures: areas of ink in the color of the brush, inside the rectangle.
    EXPECT_TRUE(calls[1].is_fill());
    EXPECT_EQ(calls[1].brush, QBrush(FillColor));
    EXPECT_TRUE(Rectangle.adjusted(-WidestBrushLine, -WidestBrushLine, WidestBrushLine, WidestBrushLine)
                    .contains(calls[1].path.boundingRect()));
}

// The tip of the brush draws a hachure: its line is not wider than the pen of a hachure, one
// pixel wide, pressed at its heaviest.
TEST(InkMedium, HachureIsDrawnWithTheTipOfTheBrush) {
    // Hatches every shape with one horizontal line through its center.
    class HorizontalHachureEngine : public IdentityGeometryEngine {
    public:
        QPainterPath hachure(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
            const QRectF bounds = shape.boundingRect();
            QPainterPath line(QPointF(bounds.left(), bounds.center().y()));
            line.lineTo(QPointF(bounds.right(), bounds.center().y()));
            return line;
        }
    };
    constexpr double WidestHachure = 2.5;
    const hpq::Theme theme = hpq::china_ink_theme(hpq::FillStyle::Hachure);

    const std::vector<DrawCall> calls = calls_of(theme, HorizontalHachureEngine(), fill_rectangle);

    ASSERT_EQ(calls.size(), 2u);
    const double height = calls[1].path.boundingRect().height();
    EXPECT_GT(height, 0.0);
    EXPECT_LT(height, WidestHachure);
}
