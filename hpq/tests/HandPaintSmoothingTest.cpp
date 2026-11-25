#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cstdint>
#include <functional>

// A rough shape is smoothed where the device painter asks for it, with QPainter::Antialiasing,
// as Qt draws: its edges then have pixels between the color of the pen and the paper.

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

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
        hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine, cache);
        QPainter painter(&device);
        painter.setRenderHint(QPainter::Antialiasing, smoothed);
        paint(painter);
        painter.end();
        target_painter.end();
        return image;
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    hpq::RoughGeometryEngine m_geometry_engine{BaseSeed};
};

TEST_F(HandPaintSmoothingTest, LineIsSmoothedWhenThePainterAsksForIt) {
    EXPECT_GT(grey_pixels(paint_by_hand(true, draw_line)), 100);
}

TEST_F(HandPaintSmoothingTest, LineStaysSharpWhenThePainterDoesNotAsk) {
    EXPECT_EQ(grey_pixels(paint_by_hand(false, draw_line)), 0);
}

TEST_F(HandPaintSmoothingTest, WideOutlineKeptByTheCacheIsSmoothed) {
    hpq::PaintCache smoothed_cache;
    hpq::PaintCache sharp_cache;

    EXPECT_GT(grey_pixels(paint_by_hand(true, draw_wide_outline, &smoothed_cache)), 500);
    EXPECT_EQ(grey_pixels(paint_by_hand(false, draw_wide_outline, &sharp_cache)), 0);
}

TEST_F(HandPaintSmoothingTest, FillIsSmoothedWhenThePainterAsksForIt) {
    EXPECT_GT(grey_pixels(paint_by_hand(true, fill_ellipse)), 300);
    EXPECT_EQ(grey_pixels(paint_by_hand(false, fill_ellipse)), 0);
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

}  // namespace
