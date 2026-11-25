#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QLinearGradient>
#include <QPainter>

#include <algorithm>
#include <cstdint>
#include <functional>

// What the state of the device painter says reaches the drawing as Qt draws it: its opacity,
// the width of its pen under its transform, and the place of its gradients.

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

constexpr int TargetSize = 300;

using Paint = std::function<void(QPainter &)>;

// What `paint` draws on a white image, with a plain painter.
QImage paint_plainly(const Paint &paint) {
    QImage image(TargetSize, TargetSize, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    paint(painter);
    return image;
}

// A horizontal gradient from black to white between x = `from` and x = `to`.
QLinearGradient black_to_white(double from, double to) {
    QLinearGradient gradient(QPointF(from, 0), QPointF(to, 0));
    gradient.setColorAt(0, Qt::black);
    gradient.setColorAt(1, Qt::white);
    return gradient;
}

class HandPaintPainterStateTest : public testing::Test {
protected:
    // What `paint` draws on a white image through a HandPaintDevice, with a cache or without.
    QImage paint_by_hand(const Paint &paint, hpq::PaintCache *cache = nullptr) const {
        QImage image(TargetSize, TargetSize, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter target_painter(&image);
        hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine, cache);
        QPainter painter(&device);
        paint(painter);
        painter.end();
        target_painter.end();
        return image;
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    hpq::RoughGeometryEngine m_geometry_engine{BaseSeed};
};

TEST_F(HandPaintPainterStateTest, OpacityLightensAFillAsQtDoes) {
    const Paint paint = [](QPainter &painter) {
        painter.setOpacity(0.3);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.drawRect(QRectF(50, 50, 200, 200));
    };

    const QRgb by_hand = paint_by_hand(paint).pixel(150, 150);

    EXPECT_NEAR(qGray(by_hand), qGray(paint_plainly(paint).pixel(150, 150)), 2);
}

TEST_F(HandPaintPainterStateTest, OpacityLightensAnOutline) {
    const QImage image = paint_by_hand([](QPainter &painter) {
        painter.setOpacity(0.3);
        painter.setPen(QPen(Qt::black, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(50, 150), QPointF(250, 150));
    });

    int darkest = 255;
    for (int y = 140; y <= 160; ++y) {
        darkest = std::min(darkest, qGray(image.pixel(150, y)));
    }
    // Drawn, but never darker than two strokes of 30 % of black over each other.
    EXPECT_LT(darkest, 255);
    EXPECT_GT(darkest, 255 * 0.7 * 0.7 - 2);
}

TEST_F(HandPaintPainterStateTest, OpacityLightensAnImageAsQtDoes) {
    QImage black(100, 100, QImage::Format_RGB32);
    black.fill(Qt::black);
    const Paint paint = [&black](QPainter &painter) {
        painter.setOpacity(0.5);
        painter.drawImage(QPointF(100, 100), black);
    };

    EXPECT_EQ(paint_by_hand(paint).pixel(150, 150), paint_plainly(paint).pixel(150, 150));
}

TEST_F(HandPaintPainterStateTest, ScaledPenIsAsWideAsItsWidthTimesTheScale) {
    // The same line on the device, drawn with a pen of 2 scaled by 3 and with a pen of 6.
    const QImage scaled = paint_by_hand([](QPainter &painter) {
        painter.scale(3, 3);
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(QPointF(10, 50), QPointF(90, 50));
    });
    const QImage wide = paint_by_hand([](QPainter &painter) {
        painter.setPen(QPen(Qt::black, 6));
        painter.drawLine(QPointF(30, 150), QPointF(270, 150));
    });

    EXPECT_EQ(scaled, wide);
}

TEST_F(HandPaintPainterStateTest, ScaledPenIsAsWideAsItsWidthTimesTheScaleWithACache) {
    hpq::PaintCache scaled_cache;
    hpq::PaintCache wide_cache;
    const QImage scaled = paint_by_hand(
        [](QPainter &painter) {
            painter.scale(3, 3);
            painter.setPen(QPen(Qt::black, 2));
            painter.drawLine(QPointF(10, 50), QPointF(90, 50));
        },
        &scaled_cache);
    const QImage wide = paint_by_hand(
        [](QPainter &painter) {
            painter.setPen(QPen(Qt::black, 6));
            painter.drawLine(QPointF(30, 150), QPointF(270, 150));
        },
        &wide_cache);

    EXPECT_EQ(scaled, wide);
}

TEST_F(HandPaintPainterStateTest, ScaledCosmeticPenKeepsItsWidth) {
    const QImage scaled = paint_by_hand([](QPainter &painter) {
        painter.scale(3, 3);
        QPen pen(Qt::black, 1);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(QPointF(10, 50), QPointF(90, 50));
    });
    const QImage unscaled = paint_by_hand([](QPainter &painter) {
        QPen pen(Qt::black, 1);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(QPointF(30, 150), QPointF(270, 150));
    });

    EXPECT_EQ(scaled, unscaled);
}

TEST_F(HandPaintPainterStateTest, GradientOfAFillFollowsTheTransform) {
    // The same rectangle and the same gradient on the device, given in two coordinate systems.
    const QImage translated = paint_by_hand([](QPainter &painter) {
        painter.translate(100, 0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(black_to_white(0, 100));
        painter.drawRect(QRectF(0, 50, 100, 100));
    });
    const QImage in_place = paint_by_hand([](QPainter &painter) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(black_to_white(100, 200));
        painter.drawRect(QRectF(100, 50, 100, 100));
    });

    EXPECT_LT(qGray(translated.pixel(110, 100)), 64);
    EXPECT_EQ(translated, in_place);
}

TEST_F(HandPaintPainterStateTest, GradientOfAFillFollowsTheBrushOrigin) {
    const QImage moved = paint_by_hand([](QPainter &painter) {
        painter.setBrushOrigin(100, 0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(black_to_white(0, 100));
        painter.drawRect(QRectF(100, 50, 100, 100));
    });
    const QImage in_place = paint_by_hand([](QPainter &painter) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(black_to_white(100, 200));
        painter.drawRect(QRectF(100, 50, 100, 100));
    });

    EXPECT_EQ(moved, in_place);
}

TEST_F(HandPaintPainterStateTest, GradientOfAPenFollowsTheTransform) {
    const QImage translated = paint_by_hand([](QPainter &painter) {
        painter.translate(100, 0);
        painter.setPen(QPen(QBrush(black_to_white(0, 100)), 4));
        painter.drawLine(QPointF(0, 100), QPointF(100, 100));
    });
    const QImage in_place = paint_by_hand([](QPainter &painter) {
        painter.setPen(QPen(QBrush(black_to_white(100, 200)), 4));
        painter.drawLine(QPointF(100, 100), QPointF(200, 100));
    });

    EXPECT_EQ(translated, in_place);
}

}  // namespace
