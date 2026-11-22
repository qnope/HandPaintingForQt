#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPainter>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QRectF Rectangle(10, 10, 100, 50);
const QColor PenColor(Qt::red);
const QColor BrushColor(Qt::blue);
constexpr double PenWidth = 2.0;

class HandPaintDeviceTest : public testing::Test {
protected:
    // Draws Rectangle on a HandPaintDevice whose target is m_target, and returns the recorded calls.
    const hpq::test::RecordingPaintEngine &draw_rectangle(const QPen &pen, const QBrush &brush) {
        QPainter target_painter(&m_target);
        hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine);
        QPainter painter(&device);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect(Rectangle);
        painter.end();
        return m_target.engine();
    }

    static QPainterPath exact_rectangle() {
        QPainterPath path;
        path.addRect(Rectangle);
        return path;
    }

    hpq::SketchTheme m_theme;
    hpq::RoughGeometryEngine m_geometry_engine;
    RecordingPaintDevice m_target;
};

TEST_F(HandPaintDeviceTest, RectangleReachesTargetAsOneRoughFillThenTwoRoughStrokes) {
    const auto &engine = draw_rectangle(QPen(PenColor, PenWidth), QBrush(BrushColor));

    const std::vector<DrawCall> &calls = engine.calls();
    ASSERT_EQ(calls.size(), 3u);

    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.color(), BrushColor);
    EXPECT_EQ(calls[0].brush.style(), Qt::SolidPattern);

    for (std::size_t index = 1; index < calls.size(); ++index) {
        EXPECT_TRUE(calls[index].is_stroke());
        EXPECT_EQ(calls[index].pen.color(), PenColor);
        EXPECT_DOUBLE_EQ(calls[index].pen.widthF(), PenWidth);
    }

    for (const DrawCall &call : calls) {
        EXPECT_NE(call.path, exact_rectangle());
    }
}

TEST_F(HandPaintDeviceTest, RectangleWithNoPenReachesTargetAsOneFillOnly) {
    const auto &engine = draw_rectangle(Qt::NoPen, QBrush(BrushColor));

    ASSERT_EQ(engine.calls().size(), 1u);
    EXPECT_TRUE(engine.calls()[0].is_fill());
}

TEST_F(HandPaintDeviceTest, RectangleWithNoBrushReachesTargetAsTwoStrokesOnly) {
    const auto &engine = draw_rectangle(QPen(PenColor, PenWidth), Qt::NoBrush);

    ASSERT_EQ(engine.calls().size(), 2u);
    EXPECT_TRUE(engine.calls()[0].is_stroke());
    EXPECT_TRUE(engine.calls()[1].is_stroke());
}

// On a high DPI target, the target painter already scales logical coordinates to device
// pixels: the hand painter must not scale them a second time.
TEST(HandPaintDeviceHighDpiTest, ShapeLandsAtTheSameLogicalPlaceOnAHighDpiTarget) {
    constexpr double DevicePixelRatio = 2.0;
    QImage target(200, 200, QImage::Format_ARGB32_Premultiplied);
    target.setDevicePixelRatio(DevicePixelRatio);
    target.fill(Qt::white);
    hpq::SketchTheme theme;
    hpq::RoughGeometryEngine geometry_engine;
    {
        QPainter target_painter(&target);
        hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
        EXPECT_EQ(device.width(), 100);
        EXPECT_EQ(device.height(), 100);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::blue);
        painter.drawRect(QRectF(40, 40, 30, 30));
    }
    // Center of the rectangle, in device pixels.
    EXPECT_EQ(target.pixelColor(110, 110), QColor(Qt::blue));
}

}  // namespace
