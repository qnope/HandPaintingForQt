#include "RecordingPaintDevice.h"

#include <hpq/HandPaintDevice.h>
#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QPainter>
#include <QPalette>

#include <vector>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QString Text("Hello");

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

// What reaches a target through a HandPaintDevice of a theme.
class InvisibleTextInkTest : public testing::Test {
protected:
    // Writes Text in `color`, and returns the calls recorded on the target.
    std::vector<DrawCall> write(const hpq::Theme &theme, const QColor &color) {
        RecordingPaintDevice target;
        QPainter target_painter(&target);
        hpq::HandPaintDevice device(target_painter, theme);
        QPainter painter(&device);
        painter.setPen(color);
        painter.drawText(10, 30, Text);
        painter.end();
        return target.engine().calls();
    }

    // Draws a line in `color`, and returns the calls recorded on the target.
    std::vector<DrawCall> draw_line(const hpq::Theme &theme, const QColor &color) {
        RecordingPaintDevice target;
        QPainter target_painter(&target);
        hpq::HandPaintDevice device(target_painter, theme);
        QPainter painter(&device);
        painter.setPen(color);
        painter.drawLine(10, 10, 100, 10);
        painter.end();
        return target.engine().calls();
    }

};

TEST_F(InvisibleTextInkTest, AThemeWritesWithAVisibleInkUnlessToldOtherwise) {
    EXPECT_EQ(hpq::Theme().text_ink, hpq::TextInk::Visible);
    EXPECT_EQ(hpq::sketch_theme().text_ink, hpq::TextInk::Visible);
}

TEST_F(InvisibleTextInkTest, TextThatIsNotSelectedIsNotDrawn) {
    const hpq::Theme theme = hpq::invisible_ink_theme();
    const QPalette palette = theme.palette;

    for (const QPalette::ColorGroup group : Groups) {
        for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
            EXPECT_TRUE(write(theme, palette.color(group, role)).empty()) << "group " << group << " role " << role;
        }
    }
    EXPECT_TRUE(write(theme, QColor(Qt::darkRed)).empty());
}

// Qt writes the text of a selection in the HighlightedText color of the group of its widget.
TEST_F(InvisibleTextInkTest, TextOfASelectionIsDrawnInItsColor) {
    for (const hpq::FillStyle fill_style : {hpq::FillStyle::Solid, hpq::FillStyle::Hachure}) {
        const hpq::Theme theme = hpq::invisible_ink_theme(fill_style);
        for (const QPalette::ColorGroup group : Groups) {
            const QColor color = theme.palette.color(group, QPalette::HighlightedText);

            const std::vector<DrawCall> calls = write(theme, color);

            ASSERT_EQ(calls.size(), 1u) << "group " << group;
            EXPECT_EQ(calls[0].kind, DrawCall::Kind::Text);
            EXPECT_EQ(calls[0].text, Text);
            EXPECT_EQ(calls[0].pen.color(), color);
        }
    }
}

// The ink hides the text only: an arrow or a check mark in the color of a text stays.
TEST_F(InvisibleTextInkTest, ShapesInTheColorOfATextAreDrawn) {
    const hpq::Theme theme = hpq::invisible_ink_theme();

    EXPECT_FALSE(draw_line(theme, theme.palette.color(QPalette::Active, QPalette::Text)).empty());
}

TEST_F(InvisibleTextInkTest, AVisibleInkDrawsTextInAnyColor) {
    const hpq::Theme theme = hpq::sketch_theme();

    EXPECT_EQ(write(theme, theme.palette.color(QPalette::Active, QPalette::Text)).size(), 1u);
    EXPECT_EQ(write(theme, QColor(Qt::darkRed)).size(), 1u);
}

}  // namespace
