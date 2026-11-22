#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>

namespace {

constexpr int ImageSize = 200;
const QPointF LineStart(20, 100);
const QPointF LineEnd(180, 100);

QPainterPath ideal_line()
{
    QPainterPath line(LineStart);
    line.lineTo(LineEnd);
    return line;
}

// Draws the line with a red pen and no brush through a device with the theme, on a red image.
QImage hand_drawn_line(const hpq::Theme& theme)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::red));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(LineStart, LineEnd);
    painter.end();
    return image;
}

} // namespace

TEST(RoughLine, SketchInkIsWithinToleranceOfTheLine)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::ink_is_near_outline(hand_drawn_line(theme), ideal_line(), theme));
}

TEST(RoughLine, SketchInkCoversEveryPointOfTheLine)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::outline_is_covered_by_ink(hand_drawn_line(theme), ideal_line(), theme));
}

TEST(RoughLine, SketchLineDiffersFromPlainLine)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::differs_from_plain_outline(hand_drawn_line(theme), ideal_line(), theme));
}

TEST(RoughLine, SketchLineHasOnlyBackgroundAndInkBlendPixels)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::has_only_background_and_ink_blend(hand_drawn_line(theme), theme));
}
