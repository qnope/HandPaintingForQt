#include <gtest/gtest.h>

#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QRgb>

#include <array>
#include <vector>

namespace {

constexpr int ImageSize = 200;
constexpr double ChannelTolerance = 2.0;
constexpr double Epsilon = 1e-9;
const QRectF Rect(20, 20, 70, 60);
const QRectF EllipseBounds(110, 20, 70, 60);
const QPolygonF Triangle({QPointF(100, 110), QPointF(170, 180), QPointF(30, 180)});

// Draws a filled rect, an ellipse and a polygon with a pen and a solid brush through a device with
// the theme, on a red image.
QImage drawing(const hpq::Theme& theme)
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(Qt::blue);
    painter.drawRect(Rect);
    painter.drawEllipse(EllipseBounds);
    painter.drawPolygon(Triangle);
    painter.end();
    return image;
}

// The blend weights of the ink and of the fill; the background weight is 1 - ink - fill.
struct Weights
{
    double ink;
    double fill;
};

// Keeps the part of the convex polygon of weights where ink_span * ink + fill_span * fill <= limit.
std::vector<Weights> clip(const std::vector<Weights>& polygon, double ink_span, double fill_span, double limit)
{
    const auto excess = [&](const Weights& point) { return ink_span * point.ink + fill_span * point.fill - limit; };
    std::vector<Weights> clipped;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const Weights& current = polygon[i];
        const Weights& next = polygon[(i + 1) % polygon.size()];
        const double current_excess = excess(current);
        const double next_excess = excess(next);
        if (current_excess <= Epsilon)
            clipped.push_back(current);
        if ((current_excess < -Epsilon && next_excess > Epsilon) || (current_excess > Epsilon && next_excess < -Epsilon))
        {
            const double t = current_excess / (current_excess - next_excess);
            clipped.push_back({current.ink + t * (next.ink - current.ink), current.fill + t * (next.fill - current.fill)});
        }
    }
    return clipped;
}

// The opaque pixel is a * background + b * ink + c * fill, with a, b, c >= 0 and a + b + c = 1,
// within the tolerance on each channel: the triangle of valid weights, clipped by the two bounds of
// each channel, is not empty.
bool is_theme_blend(QRgb pixel, const hpq::Theme& theme)
{
    if (qAlpha(pixel) != 255)
        return false;
    const QRgb background = theme.background().rgb();
    const QRgb ink = theme.ink().rgb();
    const QRgb fill = theme.fill().rgb();
    using Channel = int (*)(QRgb);
    const std::array<Channel, 3> channels = {qRed, qGreen, qBlue};
    std::vector<Weights> weights = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}};
    for (const Channel channel : channels)
    {
        // The blend minus the background on this channel is ink_span * b + fill_span * c.
        const double ink_span = channel(ink) - channel(background);
        const double fill_span = channel(fill) - channel(background);
        const double pixel_offset = channel(pixel) - channel(background);
        weights = clip(weights, ink_span, fill_span, pixel_offset + ChannelTolerance);
        weights = clip(weights, -ink_span, -fill_span, -pixel_offset + ChannelTolerance);
    }
    return !weights.empty();
}

::testing::AssertionResult has_only_theme_blends(const QImage& image, const hpq::Theme& theme)
{
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const QRgb pixel = image.pixel(x, y);
            if (!is_theme_blend(pixel, theme))
                return ::testing::AssertionFailure()
                       << "pixel (" << x << ", " << y << ") is " << QColor::fromRgba(pixel).name(QColor::HexArgb).toStdString()
                       << ", not a blend of the theme background, ink and fill";
        }
    }
    return ::testing::AssertionSuccess();
}

} // namespace

TEST(ThemeDrawing, SketchChinaInkAndChalkGivePairwiseDifferentImages)
{
    const QImage sketch = drawing(hpq::Theme::sketch());
    const QImage china_ink = drawing(hpq::Theme::china_ink());
    const QImage chalk = drawing(hpq::Theme::chalk());
    EXPECT_NE(sketch, china_ink);
    EXPECT_NE(sketch, chalk);
    EXPECT_NE(china_ink, chalk);
}

TEST(ThemeDrawing, SketchDrawingHasOnlyThemeBlends)
{
    const hpq::Theme theme = hpq::Theme::sketch();
    EXPECT_TRUE(has_only_theme_blends(drawing(theme), theme));
}

TEST(ThemeDrawing, ChinaInkDrawingHasOnlyThemeBlends)
{
    const hpq::Theme theme = hpq::Theme::china_ink();
    EXPECT_TRUE(has_only_theme_blends(drawing(theme), theme));
}

TEST(ThemeDrawing, ChalkDrawingHasOnlyThemeBlends)
{
    const hpq::Theme theme = hpq::Theme::chalk();
    EXPECT_TRUE(has_only_theme_blends(drawing(theme), theme));
}
