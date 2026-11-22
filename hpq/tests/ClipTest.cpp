#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QString>

#include <functional>

namespace {

constexpr int ImageSize = 200;
// The filled rect spans x 40..160 at y = 100.
const QRectF FilledRect(40, 40, 120, 120);
const QPoint LeftOfFilledRect(70, 100);
const QPoint RightOfFilledRect(130, 100);
// A clip that holds the left part of the filled rect, not its right part.
const QRect LeftHalf(0, 0, 100, ImageSize);
const QPointF TextPosition(20, 100);
const QString Text = QStringLiteral("Hello");

// Draws on a red image through a device with the sketch theme, with a pen and an opaque brush.
QImage hand_drawn(const std::function<void(QPainter&)>& draw)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(Qt::blue));
    draw(painter);
    painter.end();
    return image;
}

bool is_background(const QImage& image, const QPoint& pixel)
{
    return hpq_tests::is_background(image.pixel(pixel), hpq::Theme::sketch());
}

bool is_fill(const QImage& image, const QPoint& pixel)
{
    return QColor(image.pixel(pixel)) == hpq::Theme::sketch().fill();
}

QFont text_font()
{
    QFont font;
    font.setPixelSize(24);
    return font;
}

// Draws the text in the sketch ink with a plain QPainter on the sketch background, clipped to the clip.
QImage plain_text(const QRect& clip)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(hpq::Theme::sketch().background());
    QPainter painter(&image);
    painter.setClipRect(clip);
    painter.setFont(text_font());
    painter.setPen(hpq::Theme::sketch().ink());
    painter.drawText(TextPosition, Text);
    painter.end();
    return image;
}

QImage red_square()
{
    QImage square(120, 120, QImage::Format_ARGB32_Premultiplied);
    square.fill(Qt::red);
    return square;
}

} // namespace

TEST(Clip, RectOutsideTheClipRectIsNotDrawn)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, ClipRectIsMovedByTheTransformAtClipTime)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.translate(50, 0);
        // In device coordinates, the clip spans x 50..100 and the rect x 50..170.
        painter.setClipRect(QRect(0, 0, 50, ImageSize));
        painter.drawRect(QRectF(0, 40, 120, 120));
    });
    EXPECT_TRUE(is_fill(image, QPoint(75, 100)));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, LaterTransformDoesNotMoveTheClip)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.translate(-30, 0);
        // In device coordinates, the rect spans x 40..160.
        painter.drawRect(FilledRect.translated(30, 0));
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, DisabledClipDoesNotClip)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.setClipping(false);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_fill(image, RightOfFilledRect));
}

TEST(Clip, NoClipOperationRemovesTheClip)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.setClipRect(QRect(), Qt::NoClip);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, RightOfFilledRect));
}

TEST(Clip, IntersectedClipsClipToTheirIntersection)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.setClipRect(QRect(60, 0, ImageSize, ImageSize), Qt::IntersectClip);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_background(image, QPoint(50, 100)));
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, ReplacedClipClipsToTheNewClipOnly)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.setClipRect(QRect(100, 0, 100, ImageSize), Qt::ReplaceClip);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_background(image, LeftOfFilledRect));
    EXPECT_TRUE(is_fill(image, RightOfFilledRect));
}

TEST(Clip, ClipRegionClips)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRegion(QRegion(LeftHalf));
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, ClipPathClips)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        QPainterPath clip;
        clip.addRect(LeftHalf);
        painter.setClipPath(clip);
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, RestoreGivesBackTheSavedClip)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.save();
        painter.setClipRect(QRect(0, 0, 10, 10));
        painter.restore();
        painter.drawRect(FilledRect);
    });
    EXPECT_TRUE(is_fill(image, LeftOfFilledRect));
    EXPECT_TRUE(is_fill(image, RightOfFilledRect));
}

TEST(Clip, TextIsClipped)
{
    const QRect clip(0, 0, 60, ImageSize);
    const QImage image = hand_drawn([&clip](QPainter& painter) {
        painter.setClipRect(clip);
        painter.setFont(text_font());
        painter.drawText(TextPosition, Text);
    });
    EXPECT_EQ(plain_text(clip), image);
}

TEST(Clip, ImageIsClipped)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.drawImage(FilledRect.topLeft(), red_square());
    });
    EXPECT_EQ(QColor(image.pixel(LeftOfFilledRect)), QColor(Qt::red));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, PixmapIsClipped)
{
    const QImage image = hand_drawn([](QPainter& painter) {
        painter.setClipRect(LeftHalf);
        painter.drawPixmap(FilledRect.topLeft(), QPixmap::fromImage(red_square()));
    });
    EXPECT_EQ(QColor(image.pixel(LeftOfFilledRect)), QColor(Qt::red));
    EXPECT_TRUE(is_background(image, RightOfFilledRect));
}

TEST(Clip, ClipOfAPreviousPainterDoesNotRemain)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    {
        QPainter painter(&device);
        painter.setClipRect(QRect(0, 0, 10, 10));
        painter.drawRect(FilledRect);
    }
    QPainter painter(&device);
    painter.setBrush(QBrush(Qt::blue));
    painter.drawRect(FilledRect);
    painter.end();
    EXPECT_TRUE(is_fill(image, RightOfFilledRect));
}
