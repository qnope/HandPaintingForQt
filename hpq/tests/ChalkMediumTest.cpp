#include "ExactTheme.h"
#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cstdint>
#include <functional>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QColor PenColor(0xf0, 0xd0, 0x40);
const QColor ChalkFillColor(0xc0, 0x40, 0x40);
constexpr double PenWidth = 2.0;
const QLineF Line(QPointF(20, 100), QPointF(180, 100));
const QRectF Rectangle(40, 40, 100, 60);
// A chalk line is wider than its pen, and wider still where the hand presses: no more than this.
constexpr double WidestChalkLine = 8.0;

// Draws with `draw` on a HandPaintDevice of `theme`, without the wobble of the hand, in its own
// painting session, and returns the calls the target received.
std::vector<DrawCall> calls_of(const hpq::Theme &theme, const std::function<void(QPainter &)> &draw) {
    const hpq::Theme exact_theme = hpq::test::exact(theme);
    const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, exact_theme, geometry_engine);
    QPainter painter(&device);
    draw(painter);
    painter.end();
    return target.engine().calls();
}

std::vector<DrawCall> chalk_calls(const std::function<void(QPainter &)> &draw) {
    return calls_of(hpq::slate_theme(), draw);
}

void draw_line(QPainter &painter) {
    painter.setPen(QPen(PenColor, PenWidth));
    painter.drawLine(Line);
}

// Draws Rectangle without outline, filled with `color`.
std::function<void(QPainter &)> fill_rectangle(const QColor &color) {
    return [color](QPainter &painter) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRect(Rectangle);
    };
}

// The part of the pixels of `image` that are not transparent.
double coverage(const QImage &image) {
    int covered = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            covered += qAlpha(image.pixel(x, y)) > 127 ? 1 : 0;
        }
    }
    return static_cast<double>(covered) / (image.width() * image.height());
}

}  // namespace

TEST(ChalkMedium, LineIsAnAreaFilledThroughTheGrain) {
    const std::vector<DrawCall> calls = chalk_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].brush.color(), PenColor);
}

TEST(ChalkMedium, GrainOfALineIsMostlyCoveredWithHoles) {
    const std::vector<DrawCall> calls = chalk_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    const double covered = coverage(calls[0].brush.textureImage());
    EXPECT_GT(covered, 0.7);
    EXPECT_LT(covered, 0.95);
}

TEST(ChalkMedium, AreaOfALineFollowsItFromEndToEnd) {
    const std::vector<DrawCall> calls = chalk_calls(draw_line);

    ASSERT_EQ(calls.size(), 1u);
    const QRectF bounds = calls[0].path.boundingRect();
    EXPECT_LE(bounds.left(), Line.x1());
    EXPECT_GE(bounds.right(), Line.x2());
    EXPECT_GT(bounds.height(), PenWidth / 2.0);
    EXPECT_LT(bounds.height(), WidestChalkLine);
    EXPECT_LT(bounds.width(), Line.length() + WidestChalkLine);
    EXPECT_NEAR(bounds.center().y(), Line.y1(), WidestChalkLine / 2.0);
}

TEST(ChalkMedium, SameLineGivesTheSameArea) {
    const std::vector<DrawCall> first = chalk_calls(draw_line);
    const std::vector<DrawCall> second = chalk_calls(draw_line);

    ASSERT_EQ(first.size(), 1u);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_EQ(first[0].path, second[0].path);
}

TEST(ChalkMedium, PressureDiffersFromOneLineToAnother) {
    const QPointF offset(0, 40);
    const std::vector<DrawCall> first = chalk_calls(draw_line);
    const std::vector<DrawCall> second = chalk_calls([&](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth));
        painter.drawLine(Line.translated(offset));
    });

    ASSERT_EQ(first.size(), 1u);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_NE(first[0].path.translated(offset), second[0].path);
}

TEST(ChalkMedium, DashedLineKeepsItsDashesAndGetsTheGrain) {
    const std::vector<DrawCall> calls = chalk_calls([](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth, Qt::DashLine));
        painter.drawLine(Line);
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_stroke());
    EXPECT_EQ(calls[0].pen.style(), Qt::DashLine);
    EXPECT_EQ(calls[0].pen.brush().style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].pen.color(), PenColor);
    EXPECT_GT(calls[0].pen.widthF(), PenWidth);
}

TEST(ChalkMedium, FillOfTheWindowColorIsTheWholeSlate) {
    const QColor slate = hpq::slate_theme().palette().color(QPalette::Active, QPalette::Window);

    const std::vector<DrawCall> calls = chalk_calls(fill_rectangle(slate));

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(coverage(calls[0].brush.textureImage()), 1.0);
}

TEST(ChalkMedium, FillOfAnotherColorIsRubbedOnTheGrain) {
    const std::vector<DrawCall> calls = chalk_calls(fill_rectangle(ChalkFillColor));

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].brush.color(), ChalkFillColor);
    const double covered = coverage(calls[0].brush.textureImage());
    EXPECT_GT(covered, 0.3);
    EXPECT_LT(covered, 0.7);
}

TEST(ChalkMedium, GrainOfAFillIsSparserThanTheGrainOfALine) {
    const std::vector<DrawCall> line = chalk_calls(draw_line);
    const std::vector<DrawCall> fill = chalk_calls(fill_rectangle(ChalkFillColor));

    ASSERT_EQ(line.size(), 1u);
    ASSERT_EQ(fill.size(), 1u);
    EXPECT_LT(coverage(fill[0].brush.textureImage()), coverage(line[0].brush.textureImage()));
}

TEST(ChalkMedium, TextIsDrawnThroughTheGrainInThePenColor) {
    const std::vector<DrawCall> calls = chalk_calls([](QPainter &painter) {
        painter.setPen(PenColor);
        painter.drawText(10, 30, "Hello");
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].kind, DrawCall::Kind::Text);
    EXPECT_EQ(calls[0].pen.brush().style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].pen.color(), PenColor);
}

TEST(ChalkMedium, HatchedFillIsTheSlateUnderChalkLines) {
    const hpq::Theme theme = hpq::slate_theme(hpq::FillStyle::Hachure);

    const std::vector<DrawCall> calls = calls_of(theme, fill_rectangle(ChalkFillColor));

    ASSERT_EQ(calls.size(), 2u);
    // The slate, whole, under the hachures.
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.color(), theme.palette().color(QPalette::Active, QPalette::Window));
    EXPECT_EQ(coverage(calls[0].brush.textureImage()), 1.0);
    // The hachures: areas of chalk in the color of the brush, inside the rectangle.
    EXPECT_TRUE(calls[1].is_fill());
    EXPECT_EQ(calls[1].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(calls[1].brush.color(), ChalkFillColor);
    EXPECT_LT(coverage(calls[1].brush.textureImage()), 0.95);
    EXPECT_TRUE(Rectangle.adjusted(-WidestChalkLine, -WidestChalkLine, WidestChalkLine, WidestChalkLine)
                    .contains(calls[1].path.boundingRect()));
}

TEST(ChalkMedium, PencilKeepsThePlainPenAndBrush) {
    const std::vector<DrawCall> calls = calls_of(hpq::sketch_theme(), [](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth));
        painter.setBrush(ChalkFillColor);
        painter.drawRect(Rectangle);
    });

    ASSERT_GE(calls.size(), 2u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, QBrush(ChalkFillColor));
    EXPECT_TRUE(calls[1].is_stroke());
    EXPECT_EQ(calls[1].pen.brush(), QBrush(PenColor));
}
