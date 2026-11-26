#include "MakeDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/Theme.h>
#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QLinearGradient>
#include <QPainter>

#include <functional>

// By default, a rough shape is always smoothed, whether the device painter asks for it, with
// QPainter::Antialiasing, or not: its edges have pixels between the color of the pen and the
// paper. The AntialiasingMode of the theme can smooth it only where the painter asks, or never.

namespace {

constexpr int TargetSize = 300;

using Paint = std::function<void(QPainter &)>;

// The pixels of `image` that are neither black nor white.
int grey_pixels(const QImage &image) {
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const int grey = qGray(image.pixel(x, y));
            if (grey > 0 && grey < 255) {
                ++count;
            }
        }
    }
    return count;
}

void draw_line(QPainter &painter) {
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(QPointF(30, 40), QPointF(270, 200));
}

// Wider than a tile of the raster engine: it is filled in several pieces.
void draw_wide_outline(QPainter &painter) {
    painter.setPen(QPen(Qt::black, 4));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(20, 20, 260, 260));
}

void fill_ellipse(QPainter &painter) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawEllipse(QRectF(20, 20, 260, 260));
}

class HandPaintSmoothingTest : public testing::Test {
protected:
    // What `paint` draws on a white image through a HandPaintDevice whose painter asks to be
    // smoothed, or not.
    QImage paint_by_hand(bool smoothed, const Paint &paint, hpq::PaintCache *cache = nullptr) const {
        QImage image(TargetSize, TargetSize, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter target_painter(&image);
        const auto device = hpq::test::make_device(target_painter, m_theme, cache);
        QPainter painter(device.get());
        painter.setRenderHint(QPainter::Antialiasing, smoothed);
        paint(painter);
        painter.end();
        target_painter.end();
        return image;
    }

    hpq::Theme m_theme = hpq::sketch_theme();
};

TEST_F(HandPaintSmoothingTest, LineIsSmoothedWhenThePainterAsksForIt) {
    EXPECT_GT(grey_pixels(paint_by_hand(true, draw_line)), 100);
}

TEST_F(HandPaintSmoothingTest, LineIsSmoothedWhenThePainterDoesNotAsk) {
    EXPECT_EQ(paint_by_hand(false, draw_line), paint_by_hand(true, draw_line));
}

TEST_F(HandPaintSmoothingTest, WideOutlineKeptByTheCacheIsSmoothed) {
    hpq::PaintCache cache;
    hpq::PaintCache other_cache;

    EXPECT_GT(grey_pixels(paint_by_hand(false, draw_wide_outline, &cache)), 500);
    EXPECT_EQ(paint_by_hand(false, draw_wide_outline, &cache), paint_by_hand(true, draw_wide_outline, &other_cache));
}

TEST_F(HandPaintSmoothingTest, WideOutlineIsSmoothedWithoutACache) {
    EXPECT_GT(grey_pixels(paint_by_hand(false, draw_wide_outline)), 500);
}

TEST_F(HandPaintSmoothingTest, FillIsSmoothedWhetherThePainterAsksForItOrNot) {
    EXPECT_GT(grey_pixels(paint_by_hand(true, fill_ellipse)), 300);
    EXPECT_EQ(paint_by_hand(false, fill_ellipse), paint_by_hand(true, fill_ellipse));
}

TEST_F(HandPaintSmoothingTest, ThemeSmoothsAlwaysUnlessToldOtherwise) {
    EXPECT_EQ(m_theme.antialiasing_mode, hpq::AntialiasingMode::Always);
}

TEST_F(HandPaintSmoothingTest, ThemeThatNeverSmoothsDoesNotSmoothWhenThePainterAsks) {
    m_theme.antialiasing_mode = hpq::AntialiasingMode::Never;
    hpq::PaintCache cache;

    EXPECT_EQ(grey_pixels(paint_by_hand(true, draw_line)), 0);
    EXPECT_EQ(grey_pixels(paint_by_hand(true, draw_wide_outline)), 0);
    EXPECT_EQ(grey_pixels(paint_by_hand(true, draw_wide_outline, &cache)), 0);
    EXPECT_EQ(grey_pixels(paint_by_hand(true, fill_ellipse)), 0);
}

TEST_F(HandPaintSmoothingTest, ThemeThatSmoothsOnlyWhenAskedSmoothsWhereThePainterAsks) {
    const QImage always = paint_by_hand(true, draw_line);
    m_theme.antialiasing_mode = hpq::AntialiasingMode::OnlyWhenAsked;

    EXPECT_EQ(paint_by_hand(true, draw_line), always);
    EXPECT_GT(grey_pixels(paint_by_hand(true, fill_ellipse)), 300);
    EXPECT_EQ(grey_pixels(paint_by_hand(false, draw_line)), 0);
    EXPECT_EQ(grey_pixels(paint_by_hand(false, fill_ellipse)), 0);
}

TEST_F(HandPaintSmoothingTest, WideOutlineKeptByTheCacheIsSmoothedOnlyWhenAsked) {
    m_theme.antialiasing_mode = hpq::AntialiasingMode::OnlyWhenAsked;
    hpq::PaintCache cache;

    EXPECT_EQ(grey_pixels(paint_by_hand(false, draw_wide_outline, &cache)), 0);
    EXPECT_GT(grey_pixels(paint_by_hand(true, draw_wide_outline, &cache)), 500);
}

TEST_F(HandPaintSmoothingTest, SmoothedFillLeavesNoSeamBetweenItsPieces) {
    const QImage image = paint_by_hand(true, fill_ellipse);

    // Well inside the ellipse, across the cuts of the tiles: all black.
    int not_black = 0;
    for (int y = 80; y < 220; ++y) {
        for (int x = 80; x < 220; ++x) {
            not_black += qGray(image.pixel(x, y)) != 0 ? 1 : 0;
        }
    }
    EXPECT_EQ(not_black, 0);
}

// A gradient given in the coordinates of the shape spans the whole shape, also where the
// shape is filled in several pieces.
TEST(HandPaintSmoothingGradientTest, GradientOfTheShapeSpansAllItsPieces) {
    constexpr int Width = 700;
    constexpr int Height = 100;
    const hpq::Theme theme = hpq::sketch_theme();
    QImage image(Width, Height, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter target_painter(&image);
    hpq::HandPaintDevice device(target_painter, theme);
    QPainter painter(&device);
    QLinearGradient gradient(0.0, 0.5, 1.0, 0.5);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0.0, Qt::black);
    gradient.setColorAt(1.0, QColor(250, 250, 250));
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRect(QRectF(20, 20, Width - 40, Height - 40));
    painter.end();
    target_painter.end();

    // Along the middle of the shape, the grey only gets lighter: it starts again at no piece.
    int previous_grey = 0;
    int darker_steps = 0;
    for (int x = 30; x < Width - 30; ++x) {
        const int grey = qGray(image.pixel(x, Height / 2));
        darker_steps += grey < previous_grey - 1 ? 1 : 0;
        previous_grey = grey;
    }
    EXPECT_EQ(darker_steps, 0);
    EXPECT_LT(qGray(image.pixel(30, Height / 2)), 40);
    EXPECT_GT(qGray(image.pixel(Width - 31, Height / 2)), 210);
}

}  // namespace
