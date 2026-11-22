#include <gtest/gtest.h>

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>

#include <memory>

namespace {

constexpr int ImageSize = 200;

// A geometry engine that gives back the ideal shape unchanged.
class IdentityGeometryEngine : public hpq::GeometryEngine
{
public:
    hpq::HandDrawnShape hand_draw(const QPainterPath& ideal, const hpq::Theme& /*theme*/,
                                  hpq::RandomGenerator& /*random*/) const override
    {
        return hpq::HandDrawnShape{ideal, ideal};
    }
};

QPolygonF triangle() { return QPolygonF({QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)}); }

QImage image_filled_with(const QColor& color)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    return image;
}

// Draws the triangle with a red pen and a blue brush through a device using the identity geometry engine.
QImage draw_triangle_through_identity_engine(const hpq::Theme& theme)
{
    QImage image = image_filled_with(Qt::red);
    hpq::HandPaintDevice device(image, theme, std::make_unique<IdentityGeometryEngine>());
    QPainter painter(&device);
    painter.setPen(QPen(Qt::red));
    painter.setBrush(QBrush(Qt::blue));
    painter.drawPolygon(triangle());
    painter.end();
    return image;
}

// Draws the triangle with a plain antialiased painter.
QImage draw_triangle_plainly(const QColor& background, const QColor& ink, double stroke_width, const QColor& fill)
{
    QImage image = image_filled_with(background);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(ink, stroke_width));
    painter.setBrush(QBrush(fill));
    painter.drawPolygon(triangle());
    painter.end();
    return image;
}

int count_different_pixels(const QImage& lhs, const QImage& rhs)
{
    int count = 0;
    for (int y = 0; y < ImageSize; ++y)
    {
        for (int x = 0; x < ImageSize; ++x)
        {
            if (lhs.pixel(x, y) != rhs.pixel(x, y))
                ++count;
        }
    }
    return count;
}

} // namespace

TEST(GeometryEngine, IdentityEngineWithSketchDrawsPolygonLikePlainPainterWithSketchColors)
{
    const QImage expected = draw_triangle_plainly(QColor("#FFFFFF"), QColor("#333333"), 1.0, QColor("#D0D0D0"));

    EXPECT_EQ(0, count_different_pixels(expected, draw_triangle_through_identity_engine(hpq::Theme::sketch())));
}

TEST(GeometryEngine, IdentityEngineWithChinaInkDrawsPolygonLikePlainPainterWithChinaInkColors)
{
    const QImage expected = draw_triangle_plainly(QColor("#F5EFE0"), QColor("#000000"), 2.5, QColor("#808080"));

    EXPECT_EQ(0, count_different_pixels(expected, draw_triangle_through_identity_engine(hpq::Theme::china_ink())));
}
