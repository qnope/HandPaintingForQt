#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QRegion>
#include <QWidget>

namespace {

constexpr int ImageSize = 200;

// A 100x80 widget that draws the outline of the rect (10,10,80,60) with a pen and no brush.
class RectWidget : public QWidget
{
public:
    RectWidget() { resize(100, 80); }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);
        painter.setPen(QPen(Qt::black));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(10, 10, 80, 60);
    }
};

// Renders the widget at (50,50) through a device with the theme, on a red image.
QImage rendered_widget(const hpq::Theme& theme)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    RectWidget widget;
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
