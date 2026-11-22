#include "StyleRendering.h"

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QRadioButton>

namespace {

using hpq::test::has_image;
using hpq::test::render_with_style;

constexpr int HalfProgress = 50;

}  // namespace

TEST(OtherWidgetsTest, UncheckedCheckBoxHasNoImage) {
    QCheckBox check_box("Check box");
    check_box.setChecked(false);

    EXPECT_FALSE(has_image(render_with_style(check_box)));
}

TEST(OtherWidgetsTest, CheckedCheckBoxHasNoImage) {
    QCheckBox check_box("Check box");
    check_box.setChecked(true);

    EXPECT_FALSE(has_image(render_with_style(check_box)));
}

TEST(OtherWidgetsTest, UncheckedRadioButtonHasNoImage) {
    QRadioButton radio_button("Radio button");
    radio_button.setChecked(false);

    EXPECT_FALSE(has_image(render_with_style(radio_button)));
}

TEST(OtherWidgetsTest, CheckedRadioButtonHasNoImage) {
    QRadioButton radio_button("Radio button");
    radio_button.setChecked(true);

    EXPECT_FALSE(has_image(render_with_style(radio_button)));
}

TEST(OtherWidgetsTest, LineEditWithTextHasNoImage) {
    QLineEdit line_edit("Some text");

    EXPECT_FALSE(has_image(render_with_style(line_edit)));
}

TEST(OtherWidgetsTest, ProgressBarAtHalfHasNoImage) {
    QProgressBar progress_bar;
    progress_bar.setRange(0, 100);
    progress_bar.setValue(HalfProgress);

    EXPECT_FALSE(has_image(render_with_style(progress_bar)));
}
