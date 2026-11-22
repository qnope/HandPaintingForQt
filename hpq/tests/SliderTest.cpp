#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPalette>
#include <QSlider>

#include <algorithm>
#include <vector>

using hpq::test::DrawCall;

namespace {

constexpr int SliderLength = 180;
constexpr int SliderThickness = 20;

void set_up(QSlider &slider, Qt::Orientation orientation) {
    slider.setOrientation(orientation);
    slider.setRange(0, 100);
    slider.setValue(30);
    if (orientation == Qt::Horizontal) {
        slider.resize(SliderLength, SliderThickness);
    } else {
        slider.resize(SliderThickness, SliderLength);
    }
}

QRectF device_bounds(const DrawCall &call) {
    return call.transform.mapRect(call.path.boundingRect());
}

bool is_handle(const DrawCall &call, const QPalette &palette) {
    return call.kind == DrawCall::Kind::Path && call.brush.style() == Qt::SolidPattern &&
           call.brush.color() == palette.color(QPalette::Active, QPalette::Button) &&
           call.pen.style() != Qt::NoPen && call.pen.color() == palette.color(QPalette::Active, QPalette::Dark);
}

// A shape that runs along most of the slider but is thinner than it: the groove, not the background.
bool is_groove(const DrawCall &call, Qt::Orientation orientation) {
    if (call.kind != DrawCall::Kind::Path || call.path.isEmpty()) {
        return false;
    }
    const QRectF bounds = device_bounds(call);
    const qreal along = orientation == Qt::Horizontal ? bounds.width() : bounds.height();
    const qreal across = orientation == Qt::Horizontal ? bounds.height() : bounds.width();
    return along >= SliderLength / 2 && across < SliderThickness;
}

void expect_vector_groove_and_handle(Qt::Orientation orientation) {
    QSlider slider;
    set_up(slider, orientation);
    const QPalette palette = hpq::sketch_theme().palette();

    const std::vector<DrawCall> calls = hpq::test::render_with_style(slider);

    EXPECT_FALSE(hpq::test::has_image(calls));
    EXPECT_TRUE(std::any_of(calls.begin(), calls.end(),
                            [orientation](const DrawCall &call) { return is_groove(call, orientation); }));
    EXPECT_TRUE(std::any_of(calls.begin(), calls.end(),
                            [&palette](const DrawCall &call) { return is_handle(call, palette); }));
}

// The handle outline, pen included, lies inside the slider: no side of it is cut off.
void expect_handle_outline_inside(QSlider &slider) {
    const QPalette palette = hpq::sketch_theme().palette();
    const std::vector<DrawCall> calls = hpq::test::render_with_style(slider);

    const auto handle = std::find_if(calls.begin(), calls.end(),
                                     [&palette](const DrawCall &call) { return is_handle(call, palette); });
    ASSERT_NE(handle, calls.end());
    const qreal half_pen = std::max<qreal>(handle->pen.widthF(), 1.0) / 2;
    const QRectF outline = device_bounds(*handle).adjusted(-half_pen, -half_pen, half_pen, half_pen);
    EXPECT_TRUE(QRectF(slider.rect()).contains(outline))
        << "outline " << outline.x() << "," << outline.y() << " " << outline.width() << "x" << outline.height()
        << " outside slider " << slider.width() << "x" << slider.height();
}

void expect_handle_outline_inside(Qt::Orientation orientation) {
    QSlider slider;
    set_up(slider, orientation);
    expect_handle_outline_inside(slider);
}

void expect_handle_outline_inside_at_size_hint(Qt::Orientation orientation) {
    QSlider slider;
    set_up(slider, orientation);
    slider.resize(slider.sizeHint());
    expect_handle_outline_inside(slider);
}

}  // namespace

TEST(SliderTest, HorizontalSliderDrawsGrooveAndHandleAsVectorShapes) {
    expect_vector_groove_and_handle(Qt::Horizontal);
}

TEST(SliderTest, VerticalSliderDrawsGrooveAndHandleAsVectorShapes) {
    expect_vector_groove_and_handle(Qt::Vertical);
}

TEST(SliderTest, HorizontalSliderHandleOutlineIsInsideTheSlider) {
    expect_handle_outline_inside(Qt::Horizontal);
}

TEST(SliderTest, VerticalSliderHandleOutlineIsInsideTheSlider) {
    expect_handle_outline_inside(Qt::Vertical);
}

TEST(SliderTest, HorizontalSliderHandleOutlineIsInsideTheSliderAtSizeHint) {
    expect_handle_outline_inside_at_size_hint(Qt::Horizontal);
}

TEST(SliderTest, VerticalSliderHandleOutlineIsInsideTheSliderAtSizeHint) {
    expect_handle_outline_inside_at_size_hint(Qt::Vertical);
}
