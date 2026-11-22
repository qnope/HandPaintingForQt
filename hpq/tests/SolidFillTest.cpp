#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <vector>

namespace {

constexpr int ImageSize = 200;
constexpr int BlendTolerance = 3;
const QRectF Rect(40, 40, 120, 120);
const double HalfPixelDiagonal = std::sqrt(2.0) / 2.0;
const QPolygonF Triangle({QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)});

// Draws with the pen and brush through a device with the theme, on a red image.
QImage hand_drawn(const hpq::Theme& theme, const QPen& pen, const QBrush& brush,
                  const std::function<void(QPainter&)>& draw)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setPen(pen);
    painter.setBrush(brush);
    draw(painter);
    painter.end();
    return image;
}

QImage hand_drawn_rect(const hpq::Theme& theme, const QPen& pen, const QBrush& brush)
{
    return hand_drawn(theme, pen, brush, [](QPainter& painter) { painter.drawRect(Rect); });
}

QImage filled_rect_with_pen(const hpq::Theme& theme)
{
    return hand_drawn_rect(theme, QPen(Qt::black), QBrush(Qt::blue));
}

// The rect drawn by a plain antialiased QPainter with the theme ink, stroke width and fill.
QImage plain_filled_rect(const hpq::Theme& theme)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(theme.background());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(theme.ink(), theme.stroke_width()));
    painter.setBrush(theme.fill());
    painter.drawRect(Rect);
    painter.end();
    return image;
}

bool is_fill(QRgb pixel, const hpq::Theme& theme) { return pixel == theme.fill().rgba(); }

// Every pixel farther than D outside the rect is a background pixel.
::testing::AssertionResult is_background_far_outside_rect(const QImage& image, const hpq::Theme& theme)
{
    const double tolerance = hpq_tests::outline_tolerance(theme);
    const QRectF reach = Rect.adjusted(-tolerance - 1, -tolerance - 1, tolerance + 1, tolerance + 1);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (!reach.intersects(QRectF(x, y, 1, 1)) && !hpq_tests::is_background(image.pixel(x, y), theme))
                return ::testing::AssertionFailure() << "pixel (" << x << ", " << y << ") is not background";
        }
    }
    return ::testing::AssertionSuccess();
}

// Ink drawn over the fill or the background: darker than the theme fill. Fill pixels, background
// pixels and blends of the fill over the background are not.
bool is_darker_than_fill(QRgb pixel, const hpq::Theme& theme) { return qGray(pixel) < qGray(theme.fill().rgb()); }

// The pixel may be within the distance of the point (same convention as HandDrawnCheck).
bool pixel_is_within(int x, int y, const QPointF& point, double distance)
{
    return QLineF(QPointF(x + 0.5, y + 0.5), point).length() <= distance + HalfPixelDiagonal;
}

bool has_ink_over_fill_within(const QImage& image, const QPointF& point, double distance, const hpq::Theme& theme)
{
    const int reach = static_cast<int>(std::ceil(distance + HalfPixelDiagonal));
    const int x_center = static_cast<int>(std::floor(point.x()));
    const int y_center = static_cast<int>(std::floor(point.y()));
    for (int y = std::max(0, y_center - reach); y <= std::min(image.height() - 1, y_center + reach); ++y)
    {
        for (int x = std::max(0, x_center - reach); x <= std::min(image.width() - 1, x_center + reach); ++x)
        {
            if (is_darker_than_fill(image.pixel(x, y), theme) && pixel_is_within(x, y, point, distance))
                return true;
        }
    }
    return false;
}

// The points of the rect outline, one every pixel along each side, corners included.
std::vector<QPointF> rect_outline_samples()
{
    const QPolygonF corners({Rect.topLeft(), Rect.topRight(), Rect.bottomRight(), Rect.bottomLeft(), Rect.topLeft()});
    std::vector<QPointF> samples;
    for (int i = 1; i < corners.size(); ++i)
    {
        const QLineF side(corners[i - 1], corners[i]);
        const int steps = static_cast<int>(std::ceil(side.length()));
        for (int step = 0; step <= steps; ++step)
            samples.push_back(side.pointAt(static_cast<double>(step) / steps));
    }
    return samples;
}

// Every point of the rect outline has ink over the fill within D.
::testing::AssertionResult outline_is_covered_by_ink_over_fill(const QImage& image, const hpq::Theme& theme)
{
    const double tolerance = hpq_tests::outline_tolerance(theme);
    for (const QPointF& point : rect_outline_samples())
    {
        if (!has_ink_over_fill_within(image, point, tolerance, theme))
            return ::testing::AssertionFailure() << "outline point (" << point.x() << ", " << point.y()
                                                 << ") has no pixel darker than the fill within D = " << tolerance;
    }
    return ::testing::AssertionSuccess();
}

// The band around the outline, between D + 1 inside and D outside the rect, holds ink over the fill.
::testing::AssertionResult band_has_ink_over_fill(const QImage& image, const hpq::Theme& theme)
{
    const double tolerance = hpq_tests::outline_tolerance(theme);
    const QRectF outer = Rect.adjusted(-tolerance, -tolerance, tolerance, tolerance);
    const QRectF inner = Rect.adjusted(tolerance + 1, tolerance + 1, -tolerance - 1, -tolerance - 1);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const QRectF pixel_area(x, y, 1, 1);
            if (outer.intersects(pixel_area) && !inner.contains(pixel_area)
                && is_darker_than_fill(image.pixel(x, y), theme))
                return ::testing::AssertionSuccess();
        }
    }
    return ::testing::AssertionFailure() << "the band around the outline holds only fill, background or their blends";
}

// The inside is the fill, the outline has ink over the fill all along it, and the image is not
// the plain QPainter drawing.
::testing::AssertionResult is_hand_drawn_outline_over_fill(const QImage& image, const hpq::Theme& theme)
{
    const double tolerance = hpq_tests::outline_tolerance(theme);
    const QRectF inner = Rect.adjusted(tolerance + 1, tolerance + 1, -tolerance - 1, -tolerance - 1);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (inner.contains(QRectF(x, y, 1, 1)) && !is_fill(image.pixel(x, y), theme))
                return ::testing::AssertionFailure() << "inner pixel (" << x << ", " << y << ") is not the fill";
        }
    }
    for (auto check : {band_has_ink_over_fill, outline_is_covered_by_ink_over_fill})
    {
        if (::testing::AssertionResult result = check(image, theme); !result)
            return result;
    }
    if (image == plain_filled_rect(theme))
        return ::testing::AssertionFailure() << "the image equals the plain QPainter drawing of the rect";
    return ::testing::AssertionSuccess();
}

bool is_near(int value, int expected) { return std::abs(value - expected) <= BlendTolerance; }

QBrush half_transparent_blue()
{
    QColor blue(Qt::blue);
    blue.setAlpha(128);
    return QBrush(blue);
}

// Every pixel of the image satisfies the predicate.
::testing::AssertionResult every_pixel(const QImage& image, const std::function<bool(QRgb)>& predicate)
{
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (!predicate(image.pixel(x, y)))
                return ::testing::AssertionFailure() << "pixel (" << x << ", " << y << ") fails";
        }
    }
    return ::testing::AssertionSuccess();
}

} // namespace

TEST(SolidFill, SketchFilledRectCenterHasThemeFillColor)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_EQ(QColor(filled_rect_with_pen(theme).pixel(100, 100)), QColor("#D0D0D0"));
}

TEST(SolidFill, SketchFilledRectIsBackgroundFartherThanDOutside)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(is_background_far_outside_rect(filled_rect_with_pen(theme), theme));
}

TEST(SolidFill, SketchFilledRectHasHandDrawnOutline)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(is_hand_drawn_outline_over_fill(filled_rect_with_pen(theme), theme));
}

TEST(SolidFill, SketchHalfTransparentBrushBlendsThemeFillAtHalf)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QColor center(hand_drawn_rect(theme, Qt::NoPen, half_transparent_blue()).pixel(100, 100));
    // 50% of #D0D0D0 over #FFFFFF.
    const int expected = (0xD0 + 0xFF) / 2;
    EXPECT_TRUE(is_near(center.red(), expected) && is_near(center.green(), expected)
                && is_near(center.blue(), expected))
        << center.name().toStdString();
}

TEST(SolidFill, SketchHalfTransparentBrushWithoutPenHasNoInk)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = hand_drawn_rect(theme, Qt::NoPen, half_transparent_blue());
    EXPECT_TRUE(every_pixel(image, [&theme](QRgb pixel) { return pixel != theme.ink().rgba(); }));
}

TEST(SolidFill, NoPenNoBrushShapesLeaveOnlyBackground)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = hand_drawn(theme, Qt::NoPen, Qt::NoBrush, [](QPainter& painter) {
        painter.drawRect(Rect);
        painter.drawEllipse(Rect);
        painter.drawPolygon(Triangle);
    });
    EXPECT_TRUE(every_pixel(image, [&theme](QRgb pixel) { return hpq_tests::is_background(pixel, theme); }));
}
