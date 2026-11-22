#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QPushButton>
#include <QToolButton>

#include <algorithm>
#include <vector>

namespace {

using hpq::test::DrawCall;
using hpq::test::has_image;
using hpq::test::render_with_style;

bool has_button_fill(const std::vector<DrawCall> &calls) {
    const QColor button_color = hpq::sketch_theme().palette().color(QPalette::Button);
    return std::any_of(calls.begin(), calls.end(), [&](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Path && call.brush.style() == Qt::SolidPattern &&
               call.brush.color() == button_color;
    });
}

bool has_text(const std::vector<DrawCall> &calls, const QString &text) {
    return std::any_of(calls.begin(), calls.end(), [&](const DrawCall &call) {
        return call.kind == DrawCall::Kind::Text && call.text == text;
    });
}

void expect_flat_button(QWidget &button, const QString &text) {
    button.resize(button.sizeHint());
    const std::vector<DrawCall> calls = render_with_style(button);

    EXPECT_FALSE(has_image(calls));
    EXPECT_TRUE(has_button_fill(calls));
    EXPECT_TRUE(has_text(calls, text));
}

}  // namespace

TEST(ButtonWidgetsTest, PushButtonIsAFlatPanelWithItsText) {
    QPushButton button("OK");

    expect_flat_button(button, "OK");
}

TEST(ButtonWidgetsTest, ToolButtonIsAFlatPanelWithItsText) {
    QToolButton button;
    button.setText("Tool");
    button.setToolButtonStyle(Qt::ToolButtonTextOnly);
    button.setAutoRaise(false);

    expect_flat_button(button, "Tool");
}
