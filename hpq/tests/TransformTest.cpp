#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>

#include <functional>

namespace {

constexpr int ImageSize = 200;

// Draws with a pen and no brush through a device with the theme, on a red image.
QImage hand_drawn(const hpq::Theme& theme, const std::function<void(QPainter&)>& draw)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(Qt::NoBrush);
    draw(painter);
    painter.end();
    return image;
}

QPainterPath rect_outline(const QRectF& rect)
{
    QPainterPath outline;
    outline.addRect(rect);
    return outline;
}

QImage translated_rect(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) {
        painter.translate(50, 0);
        painter.drawRect(QRectF(20, 40, 80, 80));
    });
}

QImage scaled_rect(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) {
        painter.scale(2, 2);
        painter.drawRect(QRectF(20, 20, 40, 40));
    });
}

} // namespace

TEST(Transform, TranslatedRectIsHandDrawnOutlineOfTranslatedRect)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(translated_rect(theme), rect_outline(QRectF(70, 40, 80, 80)), theme));
}

TEST(Transform, TranslatedRectLeavesUntranslatedLeftEdgeBackground)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_background(translated_rect(theme).pixel(20, 80), theme));
}

TEST(Transform, ScaledRectIsHandDrawnOutlineOfScaledRect)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(scaled_rect(theme), rect_outline(QRectF(40, 40, 80, 80)), theme));
}
