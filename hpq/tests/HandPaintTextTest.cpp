#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPainter>

#include <cmath>

#include <cstdint>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QColor PenColor(Qt::darkRed);
const QString Text("Hello");
const QString ThemeFontFamily("Architects Daughter");
constexpr int PointsPerInch = 72;

class HandPaintTextTest : public testing::Test {
protected:
    // Draws Text with `font` and PenColor on a HandPaintDevice whose target is m_target,
    // and returns the calls recorded on the target.
    const std::vector<DrawCall> &draw_text(const QFont &font) {
        QPainter target_painter(&m_target);
        hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine);
        QPainter painter(&device);
        painter.setPen(PenColor);
        painter.setFont(font);
        painter.drawText(10, 30, Text);
        painter.end();
        m_device_dpi_y = device.logicalDpiY();
        return m_target.engine().calls();
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    hpq::RoughGeometryEngine m_geometry_engine{BaseSeed};
    RecordingPaintDevice m_target;
    int m_device_dpi_y = 0;
};

TEST_F(HandPaintTextTest, TextReachesTargetInThemeFontAtSamePixelSizeInPenColor) {
    QFont font("Helvetica");
    font.setPixelSize(16);

    const std::vector<DrawCall> &calls = draw_text(font);

    ASSERT_EQ(calls.size(), 1u);
    const DrawCall &call = calls[0];
    EXPECT_EQ(call.kind, DrawCall::Kind::Text);
    EXPECT_EQ(call.text, Text);
    EXPECT_EQ(call.font.family(), ThemeFontFamily);
    EXPECT_EQ(call.font.pixelSize(), 16);
    EXPECT_EQ(call.pen.color(), PenColor);
}

TEST_F(HandPaintTextTest, PointSizeBecomesSamePixelSizeOnDevice) {
    QFont font("Helvetica");
    font.setPointSize(12);

    const std::vector<DrawCall> &calls = draw_text(font);

    ASSERT_EQ(calls.size(), 1u);
    const int expected_pixel_size = static_cast<int>(std::lround(12.0 * m_device_dpi_y / PointsPerInch));
    EXPECT_EQ(calls[0].font.family(), ThemeFontFamily);
    EXPECT_EQ(calls[0].font.pixelSize(), expected_pixel_size);
}

}  // namespace
