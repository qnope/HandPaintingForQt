#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

#include <optional>
#include <utility>

namespace {

constexpr int ImageSize = 200;
const QRectF Rect(20, 20, 70, 60);
const QRectF EllipseBounds(110, 20, 70, 60);
const QPolygonF Triangle({QPointF(100, 110), QPointF(170, 180), QPointF(30, 180)});

QImage blank_image()
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    return image;
}

// Draws a rect, an ellipse and a polygon with a pen and a brush through the device.
void draw_shapes(hpq::HandPaintDevice& device)
{
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(Qt::black);
    painter.drawRect(Rect);
    painter.drawEllipse(EllipseBounds);
    painter.drawPolygon(Triangle);
    painter.end();
}

// Draws a rect with QPainter's default state (pen, no brush, no transform), then changes that state:
// a second painter on the same device must start again from the defaults.
void draw_default_state_rect_then_changed_state_shapes(hpq::HandPaintDevice& device)
{
    QPainter painter(&device);
    painter.drawRect(Rect);
    painter.setBrush(Qt::blue);
    painter.drawEllipse(EllipseBounds);
    painter.translate(10, 5);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(Triangle);
    painter.end();
}

// Paints the drawing twice on the same device with the seed and gives the image after each paint.
template <typename Drawing>
std::pair<QImage, QImage> painted_twice_on_one_device(Drawing drawing, hpq::Seed seed)
{
    QImage image = blank_image();
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    device.set_seed(seed);
    drawing(device);
    const QImage first_paint = image;
    drawing(device);
    return {first_paint, image};
}

// Draws the shapes on a new device, with the seed when there is one.
QImage drawn(std::optional<hpq::Seed> seed)
{
    QImage image = blank_image();
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    if (seed)
        device.set_seed(*seed);
    draw_shapes(device);
    return image;
}

QImage drawn_with_seed(hpq::Seed seed) { return drawn(seed); }

QImage drawn_without_seed() { return drawn(std::nullopt); }

} // namespace

TEST(Seed, SameSeedOnTwoDevicesGivesIdenticalImages)
{
    EXPECT_EQ(drawn_with_seed(42), drawn_with_seed(42));
}

TEST(Seed, DifferentSeedsGiveDifferentImages)
{
    EXPECT_NE(drawn_with_seed(1), drawn_with_seed(2));
}

TEST(Seed, DevicesWithoutSeedGiveIdenticalImages)
{
    EXPECT_EQ(drawn_without_seed(), drawn_without_seed());
}

TEST(Seed, DeviceWithoutSeedUsesTheDefaultSeed)
{
    EXPECT_EQ(drawn_without_seed(), drawn_with_seed(hpq::HandPaintDevice::DefaultSeed));
}

TEST(Seed, RepaintingTheSameDeviceGivesTheSameImage)
{
    const auto [first_paint, second_paint] = painted_twice_on_one_device(draw_shapes, 7);
    EXPECT_EQ(second_paint, first_paint);
}

TEST(Seed, RepaintingTheSameDeviceStartsFromTheDefaultPainterState)
{
    const auto [first_paint, second_paint] =
        painted_twice_on_one_device(draw_default_state_rect_then_changed_state_shapes, 7);
    EXPECT_EQ(second_paint, first_paint);
}
