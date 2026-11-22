#include "HandDrawnCheck.h"
#include "RectWidget.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QImage>
#include <QPainterPath>
#include <QPoint>
#include <QRectF>
#include <QRegion>

namespace {

constexpr int ImageSize = 200;

// Renders the widget at (50,50) through a device with the theme, on a red image.
QImage rendered_widget(const hpq::Theme& theme)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    hpq_tests::RectWidget widget;
    widget.render(&device, QPoint(50, 50), QRegion(), QWidget::DrawChildren);
    return image;
}

QPainterPath rect_outline(const QRectF& rect)
{
    QPainterPath outline;
    outline.addRect(rect);
    return outline;
}

} // namespace

TEST(WidgetRender, RenderedWidgetRectIsHandDrawnOutlineAtOffset)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(
        hpq_tests::is_hand_drawn_outline(rendered_widget(theme), rect_outline(QRectF(60, 60, 80, 60)), theme));
}

TEST(WidgetRender, RenderedWidgetLeavesPixelOutsideTheRectBackground)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_background(rendered_widget(theme).pixel(20, 20), theme));
}
