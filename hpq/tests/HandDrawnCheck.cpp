#include "HandDrawnCheck.h"

#include <hpq/Theme.h>

#include <QColor>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <vector>

namespace hpq_tests {

namespace {

constexpr int ChannelTolerance = 2;
const double HalfPixelDiagonal = std::sqrt(2.0) / 2.0;

QPointF pixel_center(int x, int y) { return QPointF(x + 0.5, y + 0.5); }

double distance_to_segment(const QPointF& point, const QLineF& segment)
{
    const QPointF direction = segment.p2() - segment.p1();
    const double squared_length = QPointF::dotProduct(direction, direction);
    double t = 0.0;
    if (squared_length > 0.0)
        t = std::clamp(QPointF::dotProduct(point - segment.p1(), direction) / squared_length, 0.0, 1.0);
    const QPointF nearest = segment.p1() + t * direction;
    return QLineF(point, nearest).length();
}

// The ideal outline as straight segments (curves flattened).
std::vector<QLineF> outline_segments(const QPainterPath& ideal_outline)
{
    std::vector<QLineF> segments;
    for (const QPolygonF& polygon : ideal_outline.toSubpathPolygons())
    {
        for (int i = 1; i < polygon.size(); ++i)
            segments.emplace_back(polygon[i - 1], polygon[i]);
    }
    return segments;
}

double distance_to_outline(const QPointF& point, const std::vector<QLineF>& segments)
{
    double distance = std::numeric_limits<double>::max();
    for (const QLineF& segment : segments)
        distance = std::min(distance, distance_to_segment(point, segment));
    return distance;
}

bool pixel_is_within(int x, int y, const QPointF& point, double distance)
{
    return QLineF(pixel_center(x, y), point).length() <= distance + HalfPixelDiagonal;
}

// The outline points, one every pixel along each segment, both ends included.
std::vector<QPointF> outline_samples(const std::vector<QLineF>& segments)
{
    std::vector<QPointF> samples;
    for (const QLineF& segment : segments)
    {
        const int steps = std::max(1, static_cast<int>(std::ceil(segment.length())));
        for (int i = 0; i <= steps; ++i)
            samples.push_back(segment.pointAt(static_cast<double>(i) / steps));
    }
    return samples;
}

bool has_ink_pixel_within(const QImage& image, const QPointF& point, double distance, const hpq::Theme& theme)
{
    const int reach = static_cast<int>(std::ceil(distance + HalfPixelDiagonal));
    const int x_min = std::max(0, static_cast<int>(std::floor(point.x())) - reach);
    const int x_max = std::min(image.width() - 1, static_cast<int>(std::floor(point.x())) + reach);
    const int y_min = std::max(0, static_cast<int>(std::floor(point.y())) - reach);
    const int y_max = std::min(image.height() - 1, static_cast<int>(std::floor(point.y())) + reach);
    for (int y = y_min; y <= y_max; ++y)
    {
        for (int x = x_min; x <= x_max; ++x)
        {
            const QRgb pixel = image.pixel(x, y);
            if (!is_background(pixel, theme) && is_ink_blend(pixel, theme) && pixel_is_within(x, y, point, distance))
                return true;
        }
    }
    return false;
}

QImage plain_outline(const QSize& size, const QPainterPath& ideal_outline, const hpq::Theme& theme)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(theme.background());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(theme.ink(), theme.stroke_width()));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(ideal_outline);
    painter.end();
    return image;
}

QString describe(QRgb pixel) { return QColor::fromRgba(pixel).name(QColor::HexArgb); }

} // namespace

double outline_tolerance(const hpq::Theme& theme) { return theme.roughness() + theme.stroke_width(); }

bool is_background(QRgb pixel, const hpq::Theme& theme) { return pixel == theme.background().rgba(); }

bool is_ink_blend(QRgb pixel, const hpq::Theme& theme)
{
    const QRgb background = theme.background().rgb();
    const QRgb ink = theme.ink().rgb();
    const int channels[3][3] = {{qRed(pixel), qRed(background), qRed(ink)},
                                {qGreen(pixel), qGreen(background), qGreen(ink)},
                                {qBlue(pixel), qBlue(background), qBlue(ink)}};
    // The blend ratio is read on the channel where background and ink differ the most.
    const auto* widest = std::max_element(std::begin(channels), std::end(channels), [](const auto& lhs, const auto& rhs) {
        return std::abs(lhs[2] - lhs[1]) < std::abs(rhs[2] - rhs[1]);
    });
    const int widest_span = (*widest)[2] - (*widest)[1];
    const double ratio = widest_span == 0 ? 0.0 : std::clamp(double((*widest)[0] - (*widest)[1]) / widest_span, 0.0, 1.0);
    return std::all_of(std::begin(channels), std::end(channels), [ratio](const auto& channel) {
        const double expected = channel[1] + ratio * (channel[2] - channel[1]);
        return std::abs(channel[0] - expected) <= ChannelTolerance;
    });
}

::testing::AssertionResult ink_is_near_outline(const QImage& image, const QPainterPath& ideal_outline,
                                               const hpq::Theme& theme)
{
    const std::vector<QLineF> segments = outline_segments(ideal_outline);
    const double tolerance = outline_tolerance(theme);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (is_background(image.pixel(x, y), theme))
                continue;
            const double distance = distance_to_outline(pixel_center(x, y), segments);
            if (distance > tolerance + HalfPixelDiagonal)
                return ::testing::AssertionFailure() << "pixel (" << x << ", " << y << ") is "
                                                     << describe(image.pixel(x, y)).toStdString() << " at "
                                                     << distance << " from the outline, D is " << tolerance;
        }
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult outline_is_covered_by_ink(const QImage& image, const QPainterPath& ideal_outline,
                                                     const hpq::Theme& theme)
{
    const double tolerance = outline_tolerance(theme);
    for (const QPointF& point : outline_samples(outline_segments(ideal_outline)))
    {
        if (!has_ink_pixel_within(image, point, tolerance, theme))
            return ::testing::AssertionFailure() << "outline point (" << point.x() << ", " << point.y()
                                                 << ") has no ink pixel within D = " << tolerance;
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult differs_from_plain_outline(const QImage& image, const QPainterPath& ideal_outline,
                                                      const hpq::Theme& theme)
{
    const QImage plain = plain_outline(image.size(), ideal_outline, theme);
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (image.pixel(x, y) != plain.pixel(x, y))
                return ::testing::AssertionSuccess();
        }
    }
    return ::testing::AssertionFailure() << "the image equals the plain QPainter drawing of the outline";
}

::testing::AssertionResult is_hand_drawn_outline(const QImage& image, const QPainterPath& ideal_outline,
                                                 const hpq::Theme& theme)
{
    for (auto check : {ink_is_near_outline, outline_is_covered_by_ink, differs_from_plain_outline})
    {
        ::testing::AssertionResult result = check(image, ideal_outline, theme);
        if (!result)
            return result;
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult has_only_background_and_ink_blend(const QImage& image, const hpq::Theme& theme)
{
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const QRgb pixel = image.pixel(x, y);
            if (!is_background(pixel, theme) && !is_ink_blend(pixel, theme))
                return ::testing::AssertionFailure() << "pixel (" << x << ", " << y << ") is "
                                                     << describe(pixel).toStdString()
                                                     << ", neither background nor ink blend";
        }
    }
    return ::testing::AssertionSuccess();
}

} // namespace hpq_tests
