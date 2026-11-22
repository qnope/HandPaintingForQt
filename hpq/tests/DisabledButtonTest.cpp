#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPushButton>

#include <algorithm>
#include <vector>

namespace {

using hpq::test::DrawCall;
using hpq::test::render_with_style;

// The Sketch theme palette, with Disabled panel colors that differ from the Active ones.
QPalette palette_with_distinct_disabled_colors() {
    QPalette palette = hpq::sketch_theme().palette();
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(200, 30, 30));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(30, 200, 30));
    return palette;
}

bool has_fill_color(const std::vector<DrawCall> &calls, const QColor &color) {
    return std::any_of(calls.begin(), calls.end(), [&](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Path && call.brush.style() == Qt::SolidPattern &&
               call.brush.color() == color;
    });
}

bool has_outline_color(const std::vector<DrawCall> &calls, const QColor &color) {
    return std::any_of(calls.begin(), calls.end(), [&](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Path && call.pen.style() != Qt::NoPen &&
               call.pen.color() == color;
    });
}

}  // namespace

TEST(DisabledButtonTest, DisabledPushButtonUsesTheDisabledPaletteColors) {
    const QPalette palette = palette_with_distinct_disabled_colors();
    ASSERT_NE(palette.color(QPalette::Disabled, QPalette::Button),
              palette.color(QPalette::Active, QPalette::Button));
    ASSERT_NE(palette.color(QPalette::Disabled, QPalette::Dark),
              palette.color(QPalette::Active, QPalette::Dark));
    QPushButton button("OK");
    button.setEnabled(false);
    button.resize(button.sizeHint());

    const std::vector<DrawCall> calls = render_with_style(button, palette);

    EXPECT_TRUE(has_fill_color(calls, palette.color(QPalette::Disabled, QPalette::Button)));
    EXPECT_TRUE(has_outline_color(calls, palette.color(QPalette::Disabled, QPalette::Dark)));
}
