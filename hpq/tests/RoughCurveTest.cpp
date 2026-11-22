#include "HandDrawnCheck.h"

#include <gtest/gtest.h>

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>

#include <functional>
#include <memory>
#include <ostream>

namespace {

constexpr int ImageSize = 200;
const QRectF EllipseBounds(40, 40, 120, 120);

QPainterPath ideal_ellipse()
{
    QPainterPath ellipse;
    ellipse.addEllipse(EllipseBounds);
    return ellipse;
}

QPainterPath line_and_cubic_path()
{
    QPainterPath path(QPointF(20, 150));
    path.lineTo(80, 150);
    path.cubicTo(QPointF(100, 20), QPointF(140, 20), QPointF(180, 150));
    return path;
}

QImage red_image()
{
    QImage image(ImageSize, ImageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    return image;
}

// Draws with a red pen and no brush through the device.
void draw_with_pen(hpq::HandPaintDevice& device, const std::function<void(QPainter&)>& draw)
{
    QPainter painter(&device);
    painter.setPen(QPen(Qt::red));
    painter.setBrush(Qt::NoBrush);
    draw(painter);
    painter.end();
}

// Draws with a red pen and no brush through a device with the theme, on a red image.
QImage hand_drawn(const hpq::Theme& theme, const std::function<void(QPainter&)>& draw)
{
    QImage image = red_image();
    hpq::HandPaintDevice device(image, theme);
    draw_with_pen(device, draw);
    return image;
}

void draw_ellipse(QPainter& painter) { painter.drawEllipse(EllipseBounds); }

QImage hand_drawn_ellipse(const hpq::Theme& theme)
{
    return hand_drawn(theme, draw_ellipse);
}

QImage hand_drawn_line_and_cubic(const hpq::Theme& theme)
{
    return hand_drawn(theme, [](QPainter& painter) { painter.drawPath(line_and_cubic_path()); });
}

// A geometry engine that keeps the ideal shape it is given.
class RecordingGeometryEngine : public hpq::GeometryEngine
{
public:
    explicit RecordingGeometryEngine(QPainterPath& recorded_ideal)
        : m_recorded_ideal(recorded_ideal)
    {
    }

    hpq::HandDrawnShape hand_draw(const QPainterPath& ideal, const hpq::Theme& /*theme*/,
                                  hpq::RandomGenerator& /*random*/) const override
    {
        m_recorded_ideal = ideal;
        return hpq::HandDrawnShape{ideal, ideal};
    }

private:
    QPainterPath& m_recorded_ideal;
};

struct NamedTheme
{
    const char* name;
    hpq::Theme theme;
};

void PrintTo(const NamedTheme& named_theme, std::ostream* stream) { *stream << named_theme.name; }

class RoughCurve : public ::testing::TestWithParam<NamedTheme>
{
protected:
    const hpq::Theme& theme() const { return GetParam().theme; }
};

INSTANTIATE_TEST_SUITE_P(Themes, RoughCurve,
                         ::testing::Values(NamedTheme{"Sketch", hpq::Theme::sketch()},
                                           NamedTheme{"ChinaInk", hpq::Theme::china_ink()},
                                           NamedTheme{"Chalk", hpq::Theme::chalk()}),
                         [](const ::testing::TestParamInfo<NamedTheme>& info) { return info.param.name; });

} // namespace

TEST(RoughCurveEllipse, ReachesTheGeometryEngineAsTheIdealEllipse)
{
    QImage image = red_image();
    QPainterPath recorded_ideal;
    hpq::HandPaintDevice device(image, hpq::Theme::sketch(), std::make_unique<RecordingGeometryEngine>(recorded_ideal));
    draw_with_pen(device, draw_ellipse);
    EXPECT_EQ(recorded_ideal, ideal_ellipse());
}

TEST_P(RoughCurve, EllipseIsHandDrawnOutline)
{
    EXPECT_TRUE(hpq_tests::is_hand_drawn_outline(hand_drawn_ellipse(theme()), ideal_ellipse(), theme()));
}

TEST_P(RoughCurve, EllipseCenterIsBackground)
{
    EXPECT_TRUE(hpq_tests::is_background(hand_drawn_ellipse(theme()).pixel(100, 100), theme()));
}

TEST_P(RoughCurve, EllipseHasOnlyBackgroundAndInkBlendPixels)
{
    EXPECT_TRUE(hpq_tests::has_only_background_and_ink_blend(hand_drawn_ellipse(theme()), theme()));
}

TEST_P(RoughCurve, LineAndCubicPathIsHandDrawnOutline)
{
    EXPECT_TRUE(
        hpq_tests::is_hand_drawn_outline(hand_drawn_line_and_cubic(theme()), line_and_cubic_path(), theme()));
}
