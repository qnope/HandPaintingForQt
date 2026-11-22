#include "HandDrawnCheck.h"
#include "RectWidget.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>

#include <QColor>
#include <QImage>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>

namespace {

// The pixmap at its logical size: at a device pixel ratio above 1, each logical pixel is the
// average of its physical pixels.
QImage logical_image(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QSize logical_size = pixmap.deviceIndependentSize().toSize();
    if (image.size() == logical_size)
        return image;
    QImage scaled = image.scaled(logical_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(1.0);
    return scaled;
}

// A shown rect widget with a HandPaintEffect set to Sketch.
class HandPaintEffectTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_effect = new hpq::HandPaintEffect(hpq::Theme::sketch());
        m_widget.setGraphicsEffect(m_effect); // the widget owns the effect
        m_widget.show();
    }

    QImage grabbed() { return logical_image(m_widget.grab()); }

    hpq_tests::RectWidget m_widget;
    hpq::HandPaintEffect* m_effect = nullptr;
};

QPainterPath rect_outline()
{
    QPainterPath outline;
    outline.addRect(QRectF(10, 10, 80, 60));
    return outline;
}

} // namespace

TEST_F(HandPaintEffectTest, GrabIsHandDrawnOutlineOfTheRect)
{
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(grabbed(), rect_outline(), hpq::Theme::sketch()));
}

TEST_F(HandPaintEffectTest, GrabInsideTheRectIsSketchBackground)
{
    EXPECT_EQ(QColor(grabbed().pixel(50, 40)), QColor("#FFFFFF"));
}

TEST_F(HandPaintEffectTest, ThemeIsTheGivenOne)
{
    EXPECT_EQ(m_effect->theme(), hpq::Theme::sketch());
}

TEST_F(HandPaintEffectTest, GrabAfterSetThemeChalkInsideTheRectIsChalkBackground)
{
    grabbed();
    m_effect->set_theme(hpq::Theme::chalk());
    EXPECT_EQ(QColor(grabbed().pixel(50, 40)), QColor("#2F3B33"));
}

TEST_F(HandPaintEffectTest, GrabAfterSetThemeChalkIsHandDrawnOutlineInChalkColors)
{
    grabbed();
    m_effect->set_theme(hpq::Theme::chalk());
    const QImage image = grabbed();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(image, rect_outline(), hpq::Theme::chalk()));
    EXPECT_TRUE(hpq_tests::has_only_background_and_ink_blend(image, hpq::Theme::chalk()));
}

TEST_F(HandPaintEffectTest, GrabOfAnotherWidgetTheEffectMovedToIsHandDrawnOutlineOfTheRect)
{
    hpq_tests::RectWidget other;
    other.setGraphicsEffect(m_effect); // the effect leaves m_widget; other owns it
    other.show();
    const QImage image = logical_image(other.grab());
    EXPECT_EQ(QColor(image.pixel(50, 40)), QColor("#FFFFFF"));
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(image, rect_outline(), hpq::Theme::sketch()));
}

TEST_F(HandPaintEffectTest, RenderOnImageIsHandDrawnOutlineOfTheRectOnSketchBackground)
{
    QImage image(m_widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    m_widget.render(&image);
    EXPECT_EQ(QColor(image.pixel(50, 40)), QColor("#FFFFFF"));
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(image, rect_outline(), hpq::Theme::sketch()));
}
