#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QString>

namespace {

constexpr int ImageSize = 200;
constexpr int SourceSize = 40;
const QPointF TextPosition(20, 100);
const QPointF SourcePosition(80, 80);
const QString Text = QStringLiteral("Hello");

QImage blank_image()
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    return image;
}

QFont text_font()
{
    QFont font;
    font.setPixelSize(24);
    return font;
}

QImage red_source()
{
    QImage source(SourceSize, SourceSize, QImage::Format_ARGB32_Premultiplied);
    source.fill(Qt::red);
    return source;
}

// Draws the text with a red pen through a HandPaintDevice using the theme.
QImage hand_painted_text(const hpq::Theme& theme)
{
    QImage image = blank_image();
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setFont(text_font());
    painter.setPen(Qt::red);
    painter.drawText(TextPosition, Text);
    painter.end();
    return image;
}

// Draws the text with a plain QPainter on the given background with the given pen color.
QImage plain_text(const QColor& background, const QColor& ink)
{
    QImage image = blank_image();
    image.fill(background);
    QPainter painter(&image);
    painter.setFont(text_font());
    painter.setPen(ink);
    painter.drawText(TextPosition, Text);
    painter.end();
    return image;
}

template <typename Draw>
QImage hand_painted(Draw draw)
{
    QImage image = blank_image();
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    QPainter painter(&device);
    draw(painter);
    painter.end();
    return image;
}

void expect_region_equals_source(const QImage& image, const QImage& source)
{
    const QImage region = image.copy(QRect(SourcePosition.toPoint(), source.size()));
    for (int y = 0; y < source.height(); ++y)
    {
        for (int x = 0; x < source.width(); ++x)
        {
            ASSERT_EQ(source.pixel(x, y), region.pixel(x, y)) << "pixel (" << x << ", " << y << ")";
        }
    }
}

} // namespace

TEST(PassThrough, SketchTextIsPlainTextWithSketchInk)
{
    EXPECT_EQ(plain_text(QColor("#FFFFFF"), QColor("#333333")), hand_painted_text(hpq::Theme::sketch()));
}

TEST(PassThrough, ChalkTextIsPlainTextWithChalkInk)
{
    EXPECT_EQ(plain_text(QColor("#2F3B33"), QColor("#F0F0E8")), hand_painted_text(hpq::Theme::chalk()));
}

TEST(PassThrough, ImageIsDrawnUnchanged)
{
    const QImage source = red_source();
    const QImage image = hand_painted([&](QPainter& painter) { painter.drawImage(SourcePosition, source); });
    expect_region_equals_source(image, source);
}

TEST(PassThrough, PixmapIsDrawnUnchanged)
{
    const QImage source = red_source();
    const QPixmap pixmap = QPixmap::fromImage(source);
    const QImage image = hand_painted([&](QPainter& painter) { painter.drawPixmap(SourcePosition, pixmap); });
    expect_region_equals_source(image, source);
}
