#include <hpq/RoughGeometry.h>

#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

// The seed of the hand: the tests draw the same paths at every run.
constexpr std::uint32_t HandSeed = 2024;
constexpr std::uint32_t SeedCount = 20;
// Two points this close are the same point, but for a rounding.
constexpr double Tolerance = 1e-9;

hpq::RoughOptions frame_options(double max_offset)
{
    hpq::RoughOptions options;
    options.roughness = 1.0;
    options.max_offset = max_offset;
    options.seed = HandSeed;
    return options;
}

// Rectangles of every kind: wide, thin (no wider than twice the wobble), on whole and on half
// pixels, and a shape that is not a rectangle.
std::vector<QPainterPath> frames()
{
    std::vector<QPainterPath> shapes;
    for (const QRectF &rectangle : {QRectF(10, 10, 100, 50), QRectF(-20.5, 40.5, 250, 180), QRectF(0.5, 0.5, 300, 11),
                                    QRectF(5, 5, 7, 160)}) {
        QPainterPath shape;
        shape.addRect(rectangle);
        shapes.push_back(shape);
    }
    QPainterPath ellipse;
    ellipse.addEllipse(QRectF(0, 0, 80, 40));
    shapes.push_back(ellipse);
    return shapes;
}

// The outline of `frame`, each of its strokes united with the frame by QPainterPath::united.
std::vector<QPainterPath> united_outline(const QPainterPath &frame, const hpq::RoughOptions &options, std::uint32_t seed)
{
    std::vector<QPainterPath> strokes = hpq::rough_outline(frame, options, seed);
    for (QPainterPath &stroke : strokes) {
        stroke.setFillRule(frame.fillRule());
        stroke = stroke.united(frame);
    }
    return strokes;
}

}  // namespace

// hpq unites a rectangle with its outline by a way of its own: it must give the path that
// QPainterPath::united gives, point for point. A stroke of chalk or of a brush follows the
// points of the path: other points would change its pressure.
TEST(RoughFrameOutlineTest, FrameOutlineIsTheOutlineUnitedWithTheFrame)
{
    for (const double max_offset : {2.0, 5.0}) {
        const hpq::RoughOptions options = frame_options(max_offset);
        for (const QPainterPath &frame : frames()) {
            for (std::uint32_t seed = 0; seed < SeedCount; ++seed) {
                const std::vector<QPainterPath> strokes = hpq::rough_frame_outline(frame, options, seed);
                const std::vector<QPainterPath> united = united_outline(frame, options, seed);
                ASSERT_EQ(strokes.size(), united.size());
                for (std::size_t index = 0; index < strokes.size(); ++index) {
                    const QPainterPath &stroke = strokes[index];
                    const QPainterPath &expected = united[index];
                    ASSERT_EQ(stroke.elementCount(), expected.elementCount())
                        << frame.boundingRect().x() << "," << frame.boundingRect().y() << " seed " << seed;
                    for (int element = 0; element < stroke.elementCount(); ++element) {
                        const QPainterPath::Element got = stroke.elementAt(element);
                        const QPainterPath::Element want = expected.elementAt(element);
                        ASSERT_EQ(got.type, want.type);
                        ASSERT_NEAR(got.x, want.x, Tolerance) << "seed " << seed << " element " << element;
                        ASSERT_NEAR(got.y, want.y, Tolerance) << "seed " << seed << " element " << element;
                    }
                }
            }
        }
    }
}

// A frame around the content of a widget leaves the content uncovered: no point of its
// strokes is inside it.
TEST(RoughFrameOutlineTest, FrameOutlineStaysOutOfTheFrame)
{
    const hpq::RoughOptions options = frame_options(5.0);
    const QRectF rectangle(10, 10, 100, 50);
    QPainterPath frame;
    frame.addRect(rectangle);
    const QRectF inside = rectangle.adjusted(Tolerance, Tolerance, -Tolerance, -Tolerance);
    for (std::uint32_t seed = 0; seed < SeedCount; ++seed) {
        for (const QPainterPath &stroke : hpq::rough_frame_outline(frame, options, seed)) {
            for (int element = 0; element < stroke.elementCount(); ++element) {
                const QPointF point = stroke.elementAt(element);
                EXPECT_FALSE(inside.contains(point)) << "seed " << seed << " point " << point.x() << "," << point.y();
            }
        }
    }
}
