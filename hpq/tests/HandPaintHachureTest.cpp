#include "ExactTheme.h"
#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometry.h>
#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cmath>
#include <cstdint>
#include <optional>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QRectF Rectangle(10, 10, 100, 50);
const QColor PenColor(Qt::red);
const QColor BrushColor(Qt::blue);
// How far, in levels of a channel, a pixel that mixes two colors may be from their exact mix.
constexpr double MixTolerance = 2.0;

// How much of BrushColor, from 0 to 1, `pixel` shows over `paper`; none when it is not a mix
// of the two.
std::optional<double> part_of_brush(const QColor &pixel, const QColor &paper) {
    const int pixel_levels[] = {pixel.red(), pixel.green(), pixel.blue()};
    const int paper_levels[] = {paper.red(), paper.green(), paper.blue()};
    const int brush_levels[] = {BrushColor.red(), BrushColor.green(), BrushColor.blue()};
    // Read on the channel where the brush and the paper differ the most.
    int widest = 0;
    for (int channel = 1; channel < 3; ++channel) {
        if (std::abs(brush_levels[channel] - paper_levels[channel]) >
            std::abs(brush_levels[widest] - paper_levels[widest])) {
            widest = channel;
        }
    }
    const double part = static_cast<double>(pixel_levels[widest] - paper_levels[widest]) /
                        (brush_levels[widest] - paper_levels[widest]);
    if (part < 0.0 || part > 1.0) {
        return std::nullopt;
    }
    for (int channel = 0; channel < 3; ++channel) {
        const double mix = paper_levels[channel] + part * (brush_levels[channel] - paper_levels[channel]);
        if (std::abs(pixel_levels[channel] - mix) > MixTolerance) {
            return std::nullopt;
        }
    }
    return part;
}

// The sketch theme with hachures, without the wobble of the hand: its hachures are straight
// lines on a known grid.
hpq::Theme hachure_theme() {
    return hpq::test::exact(hpq::sketch_theme(hpq::FillStyle::Hachure));
}

// Draws Rectangle with a pen and `brush` on a HandPaintDevice, and returns the calls the target received.
std::vector<DrawCall> draw_rectangle(const hpq::Theme &theme, const QBrush &brush) {
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, theme);
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
    const hpq::Theme theme = hachure_theme();
    // Without wobble, the hachures of a shape do not depend on its seed.
    const QPainterPath hachures = hpq::rough_hachure(exact_rectangle(), theme.rough_options, 0);

    const std::vector<DrawCall> calls = draw_rectangle(theme, QBrush(BrushColor));

    ASSERT_FALSE(hachures.isEmpty());
    ASSERT_EQ(calls.size(), 3u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, theme.palette.window());
    EXPECT_EQ(calls[0].path, exact_rectangle());
    EXPECT_TRUE(calls[1].is_stroke());
    EXPECT_EQ(calls[1].pen.brush(), QBrush(BrushColor));
    EXPECT_DOUBLE_EQ(calls[1].pen.widthF(), 1.0);
    EXPECT_EQ(calls[1].path, hachures);
    EXPECT_TRUE(calls[2].is_stroke());
    EXPECT_EQ(calls[2].pen.color(), PenColor);
}

TEST(HandPaintHachureTest, SolidThemeFillsWithTheBrush) {
    expect_plain_fill(draw_rectangle(hpq::test::exact(hpq::sketch_theme()), QBrush(BrushColor)), QBrush(BrushColor));
}

TEST(HandPaintHachureTest, ShapeInThePaperColorIsNotHatched) {
    const hpq::Theme theme = hachure_theme();
    const QBrush paper = theme.palette.window();

    expect_plain_fill(draw_rectangle(theme, paper), paper);
}

// A translucent brush tints what is under it: the paper of a hatched fill would hide it.
TEST(HandPaintHachureTest, TranslucentBrushIsNotHatched) {
    const QBrush tint(QColor(0, 0, 255, 100));

    expect_plain_fill(draw_rectangle(hachure_theme(), tint), tint);
}

// A shape too small for the gap of its hachures gets none: Rectangle is 50 pixels high.
TEST(HandPaintHachureTest, ShapeWithoutHachuresIsFilledWithTheBrush) {
    hpq::Theme theme = hachure_theme();
    theme.rough_options.hachure_gap = 20.0;

    expect_plain_fill(draw_rectangle(theme, QBrush(BrushColor)), QBrush(BrushColor));
}

TEST(HandPaintHachureTest, HatchedRectangleShowsThePaperBetweenLinesInTheBrush) {
    constexpr int ImageSize = 200;
    const hpq::Theme theme = hpq::sketch_theme(hpq::FillStyle::Hachure);
    const QColor paper = theme.palette.color(QPalette::Window);
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    {
        QPainter target_painter(&image);
        hpq::HandPaintDevice device(target_painter, theme);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(BrushColor);
        painter.drawRect(QRectF(20, 20, 160, 160));
    }

    // The hachures are smoothed: a pixel along their edges mixes the paper and the brush.
    double paper_pixels = 0.0;
    double brush_pixels = 0.0;
    int other_pixels = 0;
    // Well inside the rectangle, away from the ends of the hachures.
    for (int y = 40; y < 160; ++y) {
        for (int x = 40; x < 160; ++x) {
            const std::optional<double> brush_part = part_of_brush(image.pixelColor(x, y), paper);
            if (brush_part) {
                brush_pixels += *brush_part;
                paper_pixels += 1.0 - *brush_part;
            } else {
                ++other_pixels;
            }
        }
    }
    EXPECT_EQ(other_pixels, 0);
    EXPECT_GT(brush_pixels, 1000.0);
    EXPECT_GT(paper_pixels, brush_pixels);
}
