#include "RecordingPaintDevice.h"
#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QComboBox>
#include <QPalette>
#include <QPolygonF>

#include <algorithm>
#include <vector>

namespace {

using hpq::test::DrawCall;

std::vector<DrawCall> render_combo_box(bool is_editable) {
    QComboBox combo_box;
    combo_box.setEditable(is_editable);
    combo_box.addItem("A");
    combo_box.resize(120, 30);
    return hpq::test::render_with_style(combo_box);
}

std::size_t count_dark_outlines(const std::vector<DrawCall> &calls) {
    const QColor dark = hpq::sketch_theme().palette().color(QPalette::Dark);
    return std::count_if(calls.begin(), calls.end(), [&dark](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Path && call.pen.style() != Qt::NoPen && call.pen.color() == dark;
    });
}

// A down arrow (B4): one filled closed triangle in ButtonText whose lowest corner is the only one.
bool is_down_triangle(const DrawCall &call) {
    const QColor button_text = hpq::sketch_theme().palette().color(QPalette::ButtonText);
    if (!call.is_fill() || call.brush.color() != button_text) {
        return false;
    }
    const QList<QPolygonF> polygons = call.path.toSubpathPolygons();
    if (polygons.size() != 1 || !polygons.front().isClosed() || polygons.front().size() != 4) {
        return false;
    }
    QPolygonF corners = call.transform.map(polygons.front());
    corners.removeLast();
    std::vector<QPointF> sorted(corners.begin(), corners.end());
    std::sort(sorted.begin(), sorted.end(), [](QPointF first, QPointF second) { return first.y() > second.y(); });
    return sorted[0].y() > sorted[1].y();
}

std::size_t count_down_arrows(const std::vector<DrawCall> &calls) {
    return std::count_if(calls.begin(), calls.end(), is_down_triangle);
}

void expect_flat_combo_box(const std::vector<DrawCall> &calls) {
    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_EQ(count_dark_outlines(calls), 1u);
    EXPECT_EQ(count_down_arrows(calls), 1u);
}

}  // namespace

TEST(ComboBoxTest, NonEditableComboBoxIsDarkOutlineAndDownArrowWithoutImage) {
    expect_flat_combo_box(render_combo_box(false));
}

TEST(ComboBoxTest, EditableComboBoxIsDarkOutlineAndDownArrowWithoutImage) {
    expect_flat_combo_box(render_combo_box(true));
}
