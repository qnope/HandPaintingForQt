#include "RecordingPaintDevice.h"

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cstdint>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QRectF Rectangle(10, 10, 100, 50);
const QColor PenColor(Qt::red);
const QColor BrushColor(Qt::blue);

// Geometry engine that returns the shape unchanged, and one known line as its hachures.
class OneHachureGeometryEngine : public hpq::GeometryEngine {
public:
    explicit OneHachureGeometryEngine(bool hatches = true) : m_hatches(hatches) {}

    static QPainterPath hachure_line() {
        QPainterPath line(Rectangle.bottomLeft());
        line.lineTo(Rectangle.topRight());
        return line;
    }

    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &,
                                      std::uint32_t) const override {
        return {shape};
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        return shape;
    }

    QPainterPath hachure(const QPainterPath &, const hpq::RoughOptions &, std::uint32_t) const override {
        return m_hatches ? hachure_line() : QPainterPath();
    }

private:
    bool m_hatches;
};

// Draws Rectangle with a pen and `brush` on a HandPaintDevice, and returns the calls the target received.
std::vector<DrawCall> draw_rectangle(const hpq::Theme &theme, const hpq::GeometryEngine &geometry_engine,
                                     const QBrush &brush) {
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    painter.setPen(QPen(PenColor, 2.0));
    painter.setBrush(brush);
    painter.drawRect(Rectangle);
    painter.end();
    return target.engine().calls();
}

QPainterPath exact_rectangle() {
    QPainterPath path;
    path.addRect(Rectangle);
    return path;
}

// Expects one fill of the exact rectangle with `brush`, then one outline: no hachure.
void expect_plain_fill(const std::vector<DrawCall> &calls, const QBrush &brush) {
    ASSERT_EQ(calls.size(), 2u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, brush);
    EXPECT_EQ(calls[0].path, exact_rectangle());
    EXPECT_TRUE(calls[1].is_stroke());
    EXPECT_EQ(calls[1].pen.color(), PenColor);
}

}  // namespace

TEST(HandPaintHachureTest, HachureThemeFillsWithPaperThenDrawsTheHachuresInTheBrush) {
    const hpq::Theme theme = hpq::sketch_theme(hpq::FillStyle::Hachure);
    const OneHachureGeometryEngine geometry_engine;

    const std::vector<DrawCall> calls = draw_rectangle(theme, geometry_engine, QBrush(BrushColor));

    ASSERT_EQ(calls.size(), 3u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, theme.palette().window());
    EXPECT_EQ(calls[0].path, exact_rectangle());
    EXPECT_TRUE(calls[1].is_stroke());
    EXPECT_EQ(calls[1].pen.brush(), QBrush(BrushColor));
    EXPECT_DOUBLE_EQ(calls[1].pen.widthF(), 1.0);
    EXPECT_EQ(calls[1].path, OneHachureGeometryEngine::hachure_line());
    EXPECT_TRUE(calls[2].is_stroke());
    EXPECT_EQ(calls[2].pen.color(), PenColor);
}

TEST(HandPaintHachureTest, SolidThemeFillsWithTheBrush) {
    const OneHachureGeometryEngine geometry_engine;

    expect_plain_fill(draw_rectangle(hpq::sketch_theme(), geometry_engine, QBrush(BrushColor)), QBrush(BrushColor));
}

TEST(HandPaintHachureTest, ShapeInThePaperColorIsNotHatched) {
    const hpq::Theme theme = hpq::sketch_theme(hpq::FillStyle::Hachure);
    const OneHachureGeometryEngine geometry_engine;
    const QBrush paper = theme.palette().window();

    expect_plain_fill(draw_rectangle(theme, geometry_engine, paper), paper);
}

// A translucent brush tints what is under it: the paper of a hatched fill would hide it.
TEST(HandPaintHachureTest, TranslucentBrushIsNotHatched) {
    const OneHachureGeometryEngine geometry_engine;
    const QBrush tint(QColor(0, 0, 255, 100));

    expect_plain_fill(draw_rectangle(hpq::sketch_theme(hpq::FillStyle::Hachure), geometry_engine, tint), tint);
}

TEST(HandPaintHachureTest, ShapeWithoutHachuresIsFilledWithTheBrush) {
    const OneHachureGeometryEngine geometry_engine(false);

    expect_plain_fill(draw_rectangle(hpq::sketch_theme(hpq::FillStyle::Hachure), geometry_engine, QBrush(BrushColor)),
                      QBrush(BrushColor));
}

TEST(HandPaintHachureTest, HatchedRectangleShowsThePaperBetweenLinesInTheBrush) {
    constexpr int ImageSize = 200;
    const hpq::Theme theme = hpq::sketch_theme(hpq::FillStyle::Hachure);
    const QColor paper = theme.palette().color(QPalette::Window);
    const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    {
        QPainter target_painter(&image);
        hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(BrushColor);
        painter.drawRect(QRectF(20, 20, 160, 160));
    }

    int paper_pixels = 0;
    int brush_pixels = 0;
    int other_pixels = 0;
    // Well inside the rectangle, away from the ends of the hachures.
    for (int y = 40; y < 160; ++y) {
        for (int x = 40; x < 160; ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel == paper)
                ++paper_pixels;
            else if (pixel == BrushColor)
                ++brush_pixels;
            else
                ++other_pixels;
        }
    }
    EXPECT_EQ(other_pixels, 0);
    EXPECT_GT(brush_pixels, 1000);
    EXPECT_GT(paper_pixels, brush_pixels);
}
