#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QColor>
#include <QImage>
#include <QPainter>

namespace {

constexpr int ImageSize = 200;

QImage red_image()
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    return image;
}

// Begins and ends a painter on a device wrapping a red image, with no drawing.
QImage paint_nothing(const hpq::Theme& theme)
{
    QImage image = red_image();
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.end();
    return image;
}

void expect_all_pixels(const QImage& image, const QColor& color)
{
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            ASSERT_EQ(color.rgba(), image.pixel(x, y)) << "pixel (" << x << ", " << y << ")";
        }
    }
}

} // namespace

TEST(HandPaintDevice, EmptyPaintingWithSketchFillsSketchBackground)
{
    expect_all_pixels(paint_nothing(hpq::Theme::sketch()), QColor("#FFFFFF"));
}

TEST(HandPaintDevice, EmptyPaintingWithChalkFillsChalkBackground)
{
    expect_all_pixels(paint_nothing(hpq::Theme::chalk()), QColor("#2F3B33"));
}

TEST(HandPaintDevice, PainterIsActiveFromBeginUntilEnd)
{
    QImage image = red_image();
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    QPainter painter;

    EXPECT_TRUE(painter.begin(&device));
    EXPECT_TRUE(painter.isActive());
    EXPECT_TRUE(painter.end());
    EXPECT_FALSE(painter.isActive());
}
