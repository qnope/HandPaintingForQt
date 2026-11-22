#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QPolygonF>
#include <QRectF>

#include <cstdlib>
#include <functional>
#include <vector>

namespace {

constexpr int ImageSize = 200;
constexpr int BlendTolerance = 3;
constexpr double Shift = 30;
const QRectF FilledRect(40, 40, 120, 120);
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

// Draws with a pen and no brush.
QImage hand_drawn(const hpq::Theme& theme, const std::function<void(QPainter&)>& draw)
{
    return hand_drawn(theme, QPen(Qt::black), Qt::NoBrush, draw);
}

// Draws the shape after translate(Shift, 0).
QImage translated(const hpq::Theme& theme, const QPen& pen, const QBrush& brush,
                  const std::function<void(QPainter&)>& draw_shape)
{
    return hand_drawn(theme, pen, brush, [&draw_shape](QPainter& painter) {
        painter.translate(Shift, 0);
        draw_shape(painter);
    });
}

void draw_rect(QPainter& painter) { painter.drawRect(FilledRect); }
void draw_ellipse(QPainter& painter) { painter.drawEllipse(FilledRect); }
void draw_triangle(QPainter& painter) { painter.drawPolygon(Triangle); }
void draw_rect_path(QPainter& painter)
{
    QPainterPath path;
    path.addRect(FilledRect);
    painter.drawPath(path);
}

QBrush half_transparent_blue()
{
    QColor blue(Qt::blue);
    blue.setAlpha(128);
    return QBrush(blue);
}

// The theme fill blended at 50% over the background.
QColor half_fill(const hpq::Theme& theme)
{
    const QColor fill = theme.fill();
    const QColor background = theme.background();
    return QColor((fill.red() + background.red()) / 2, (fill.green() + background.green()) / 2,
                  (fill.blue() + background.blue()) / 2);
}

bool is_near(const QColor& color, const QColor& expected)
{
    return std::abs(color.red() - expected.red()) <= BlendTolerance
        && std::abs(color.green() - expected.green()) <= BlendTolerance
        && std::abs(color.blue() - expected.blue()) <= BlendTolerance;
}

// The inside pixels have the fill color and the outside pixels are background pixels.
::testing::AssertionResult is_filled_only_inside(const QImage& image, const hpq::Theme& theme, const QColor& fill,
                                                 const std::vector<QPoint>& inside,
                                                 const std::vector<QPoint>& outside)
{
    for (const QPoint& pixel : inside)
    {
        const QColor color(image.pixel(pixel));
        if (!is_near(color, fill))
            return ::testing::AssertionFailure() << "inside pixel (" << pixel.x() << ", " << pixel.y() << ") is "
                                                 << color.name().toStdString() << ", not "
                                                 << fill.name().toStdString();
    }
    for (const QPoint& pixel : outside)
    {
        if (!hpq_tests::is_background(image.pixel(pixel), theme))
            return ::testing::AssertionFailure() << "outside pixel (" << pixel.x() << ", " << pixel.y()
                                                 << ") is not background";
    }
    return ::testing::AssertionSuccess();
}

// The rect, the ellipse and the rect path translated by Shift span x 70..190 at y = 100.
const std::vector<QPoint> InsideTranslatedRect{QPoint(85, 100), QPoint(130, 100), QPoint(175, 100)};
const std::vector<QPoint> OutsideTranslatedRect{QPoint(55, 100), QPoint(196, 100)};
// The triangle translated by Shift spans x 70..190 at y = 150.
const std::vector<QPoint> InsideTranslatedTriangle{QPoint(85, 150), QPoint(130, 150), QPoint(175, 150)};
const std::vector<QPoint> OutsideTranslatedTriangle{QPoint(55, 150), QPoint(196, 150)};

QPainterPath rect_outline(const QRectF& rect)
{
    QPainterPath outline;
    outline.addRect(rect);
    return outline;
}

QImage translated_rect(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) {
        painter.translate(50, 0);
        painter.drawRect(QRectF(20, 40, 80, 80));
    });
}

QImage scaled_rect(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) {
        painter.scale(2, 2);
        painter.drawRect(QRectF(20, 20, 40, 40));
    });
}

} // namespace

TEST(Transform, TranslatedRectIsHandDrawnOutlineOfTranslatedRect)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(translated_rect(theme), rect_outline(QRectF(70, 40, 80, 80)), theme));
}

TEST(Transform, TranslatedRectLeavesUntranslatedLeftEdgeBackground)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_background(translated_rect(theme).pixel(20, 80), theme));
}

TEST(Transform, ScaledRectIsHandDrawnOutlineOfScaledRect)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(scaled_rect(theme), rect_outline(QRectF(40, 40, 80, 80)), theme));
}

TEST(Transform, TranslatedRectWithOpaqueBrushAndPenIsFilledAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, QPen(Qt::black), QBrush(Qt::blue), draw_rect);
    EXPECT_TRUE(is_filled_only_inside(image, theme, theme.fill(), InsideTranslatedRect, OutsideTranslatedRect));
}

TEST(Transform, TranslatedRectWithHalfTransparentBrushIsBlendedAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, Qt::NoPen, half_transparent_blue(), draw_rect);
    EXPECT_TRUE(is_filled_only_inside(image, theme, half_fill(theme), InsideTranslatedRect, OutsideTranslatedRect));
}

TEST(Transform, TranslatedEllipseWithOpaqueBrushAndPenIsFilledAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, QPen(Qt::black), QBrush(Qt::blue), draw_ellipse);
    EXPECT_TRUE(is_filled_only_inside(image, theme, theme.fill(), InsideTranslatedRect, OutsideTranslatedRect));
}

TEST(Transform, TranslatedEllipseWithHalfTransparentBrushAndPenIsBlendedAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, QPen(Qt::black), half_transparent_blue(), draw_ellipse);
    EXPECT_TRUE(is_filled_only_inside(image, theme, half_fill(theme), InsideTranslatedRect, OutsideTranslatedRect));
}

TEST(Transform, TranslatedPathWithOpaqueBrushAndPenIsFilledAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, QPen(Qt::black), QBrush(Qt::blue), draw_rect_path);
    EXPECT_TRUE(is_filled_only_inside(image, theme, theme.fill(), InsideTranslatedRect, OutsideTranslatedRect));
}

TEST(Transform, TranslatedPolygonWithOpaqueBrushAndPenIsFilledAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, QPen(Qt::black), QBrush(Qt::blue), draw_triangle);
    EXPECT_TRUE(
        is_filled_only_inside(image, theme, theme.fill(), InsideTranslatedTriangle, OutsideTranslatedTriangle));
}

TEST(Transform, TranslatedPolygonWithHalfTransparentBrushIsBlendedAtTranslatedPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    const QImage image = translated(theme, Qt::NoPen, half_transparent_blue(), draw_triangle);
    EXPECT_TRUE(
        is_filled_only_inside(image, theme, half_fill(theme), InsideTranslatedTriangle, OutsideTranslatedTriangle));
}

TEST(Transform, ScaledRectWithOpaqueBrushAndPenIsFilledAtScaledPlace)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    // scale(2, 2) maps the rect (20,20,40,40) to (40,40,80,80).
    const QImage image = hand_drawn(theme, QPen(Qt::black), QBrush(Qt::blue), [](QPainter& painter) {
        painter.scale(2, 2);
        painter.drawRect(QRectF(20, 20, 40, 40));
    });
    EXPECT_TRUE(is_filled_only_inside(image, theme, theme.fill(), {QPoint(50, 50), QPoint(80, 80), QPoint(110, 110)},
                                      {QPoint(30, 80), QPoint(130, 80), QPoint(80, 130)}));
}
