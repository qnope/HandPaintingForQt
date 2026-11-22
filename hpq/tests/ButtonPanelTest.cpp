#include "RecordingPaintDevice.h"
#include "StyleRendering.h"

#include <hpq/HandPaintStyle.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPainter>
#include <QStyleOption>

#include <algorithm>
#include <vector>

namespace {

using hpq::test::DrawCall;

const QRect PanelRect(10, 10, 80, 30);

std::vector<DrawCall> draw_button_panel(QStyle::State state) {
    hpq::HandPaintStyle style;
    QStyleOptionButton option;
    option.rect = PanelRect;
    option.palette = hpq::SketchTheme().palette();
    option.state = QStyle::State_Enabled | state;

    hpq::test::RecordingPaintDevice device;
    QPainter painter(&device);
    style.drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter);
    painter.end();
    return device.engine().calls();
}

bool is_closed(const QPainterPath &path) {
    const int count = path.elementCount();
    return count > 2 && QPointF(path.elementAt(0)) == QPointF(path.elementAt(count - 1));
}

bool is_inside_panel(const DrawCall &call) {
    return QRectF(PanelRect).contains(call.transform.map(call.path).boundingRect());
}

// True when one call draws a closed shape inside the panel, filled with a solid brush of fill_role
// and outlined with a pen of the Dark role.
bool has_panel_shape(const std::vector<DrawCall> &calls, QPalette::ColorRole fill_role) {
    const QPalette palette = hpq::SketchTheme().palette();
    return std::any_of(calls.begin(), calls.end(), [&](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Path && is_closed(call.path) && is_inside_panel(call) &&
               call.brush.style() == Qt::SolidPattern && call.brush.color() == palette.color(fill_role) &&
               call.pen.style() != Qt::NoPen && call.pen.color() == palette.color(QPalette::Dark);
    });
}

}  // namespace

TEST(ButtonPanelTest, RaisedPanelIsAFlatShapeOfTheButtonColor) {
    const std::vector<DrawCall> calls = draw_button_panel(QStyle::State_Raised);

    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_TRUE(has_panel_shape(calls, QPalette::Button));
}

TEST(ButtonPanelTest, SunkenPanelIsAFlatShapeOfTheMidlightColor) {
    const std::vector<DrawCall> calls = draw_button_panel(QStyle::State_Sunken);

    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_TRUE(has_panel_shape(calls, QPalette::Midlight));
}

TEST(ButtonPanelTest, CheckedPanelIsAFlatShapeOfTheMidlightColor) {
    const std::vector<DrawCall> calls = draw_button_panel(QStyle::State_On);

    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_TRUE(has_panel_shape(calls, QPalette::Midlight));
}
