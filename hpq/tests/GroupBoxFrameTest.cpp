#include "RecordingPaintDevice.h"
#include "StyleRendering.h"

#include <hpq/HandPaintStyle.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QGroupBox>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRect>
#include <QStyleOptionFrame>

#include <algorithm>
#include <vector>

namespace {

using hpq::test::DrawCall;

const QRect FrameRect(20, 30, 120, 80);

std::vector<DrawCall> draw_group_box_frame(const QPalette &palette) {
    hpq::HandPaintStyle style;
    QStyleOptionFrame option;
    option.rect = FrameRect;
    option.palette = palette;
    option.state = QStyle::State_Enabled;

    hpq::test::RecordingPaintDevice device;
    QPainter painter(&device);
    style.drawPrimitive(QStyle::PE_FrameGroupBox, &option, &painter);
    painter.end();
    return device.engine().calls();
}

bool is_rectangle(const QPainterPath &path) {
    QPainterPath rectangle;
    rectangle.addRect(path.boundingRect());
    return path == rectangle;
}

std::vector<DrawCall> strokes_of_color(const std::vector<DrawCall> &calls, const QColor &color) {
    std::vector<DrawCall> strokes;
    std::copy_if(calls.begin(), calls.end(), std::back_inserter(strokes), [&color](const DrawCall &call) {
        return call.is_stroke() && call.pen.color() == color;
    });
    return strokes;
}

}  // namespace

TEST(GroupBoxFrameTest, FrameIsOneRectangleOutlineInDarkWithoutImage) {
    const QPalette palette = hpq::SketchTheme().palette();

    const std::vector<DrawCall> calls = draw_group_box_frame(palette);

    EXPECT_FALSE(hpq::test::has_image(calls));
    ASSERT_EQ(calls.size(), 1u);
    const DrawCall &outline = calls.front();
    EXPECT_TRUE(outline.is_stroke());
    EXPECT_EQ(outline.pen.color(), palette.color(QPalette::Dark));
    EXPECT_TRUE(is_rectangle(outline.path));
    EXPECT_TRUE(QRectF(FrameRect).contains(outline.transform.mapRect(outline.path.boundingRect())));
}

TEST(GroupBoxFrameTest, TitledGroupBoxRendersOneDarkOutlineWithoutImage) {
    QGroupBox group_box("Group");
    group_box.resize(160, 120);

    const std::vector<DrawCall> calls = hpq::test::render_with_style(group_box);

    EXPECT_FALSE(hpq::test::has_image(calls));
    const QColor dark = hpq::SketchTheme().palette().color(QPalette::Dark);
    const std::vector<DrawCall> outlines = strokes_of_color(calls, dark);
    ASSERT_EQ(outlines.size(), 1u);
    EXPECT_TRUE(is_rectangle(outlines.front().path));
}
