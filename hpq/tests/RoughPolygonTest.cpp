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
#include <QPolygonF>
#include <QRectF>

#include <functional>

namespace {

constexpr int ImageSize = 200;
const QRectF Rect(40, 40, 120, 120);
const QPolygonF Triangle({QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)});
const QPolygonF Polyline({QPointF(30, 30), QPointF(100, 170), QPointF(170, 30)});

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

QImage hand_drawn_rect(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) { painter.drawRect(Rect); });
}

QImage hand_drawn_triangle(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) { painter.drawPolygon(Triangle); });
}

QImage hand_drawn_polyline(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) { painter.drawPolyline(Polyline); });
}

QPainterPath closed_outline(const QPolygonF& polygon)
{
    QPainterPath outline;
    outline.addPolygon(polygon);
    outline.closeSubpath();
    return outline;
}

QPainterPath open_outline(const QPolygonF& polygon)
{
    QPainterPath outline;
    outline.addPolygon(polygon);
    return outline;
}

} // namespace

TEST(RoughPolygon, SketchRectIsHandDrawnOutline)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    QPainterPath outline;
    outline.addRect(Rect);
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(hand_drawn_rect(theme), outline, theme));
}

TEST(RoughPolygon, SketchRectCenterIsBackground)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_background(hand_drawn_rect(theme).pixel(100, 100), theme));
}

TEST(RoughPolygon, SketchTriangleIsHandDrawnOutline)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(hand_drawn_triangle(theme), closed_outline(Triangle), theme));
}

TEST(RoughPolygon, SketchPolylineIsHandDrawnOutlineOfItsSegments)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(hand_drawn_polyline(theme), open_outline(Polyline), theme));
}

TEST(RoughPolygon, SketchPolylineHasNoClosingSegment)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_background(hand_drawn_polyline(theme).pixel(100, 30), theme));
}
