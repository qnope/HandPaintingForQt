#include <gtest/gtest.h>

#include <hpq/Theme.h>

#include <QColor>

namespace {

void expect_theme(const hpq::Theme& theme, const char* background, const char* ink, const char* fill,
                  double stroke_width, double roughness, int passes)
{
    EXPECT_EQ(QColor(background), theme.background());
    EXPECT_EQ(QColor(ink), theme.ink());
    EXPECT_EQ(QColor(fill), theme.fill());
    EXPECT_DOUBLE_EQ(stroke_width, theme.stroke_width());
    EXPECT_DOUBLE_EQ(roughness, theme.roughness());
    EXPECT_EQ(passes, theme.passes());
    EXPECT_EQ(hpq::FillStyle::Solid, theme.fill_style());
}

} // namespace

TEST(Theme, SketchPreset)
{
    expect_theme(hpq::Theme::sketch(), "#FFFFFF", "#333333", "#D0D0D0", 1.0, 1.5, 2);
}

TEST(Theme, ChinaInkPreset)
{
    expect_theme(hpq::Theme::china_ink(), "#F5EFE0", "#000000", "#808080", 2.5, 1.0, 1);
}

TEST(Theme, ChalkPreset)
{
    expect_theme(hpq::Theme::chalk(), "#2F3B33", "#F0F0E8", "#A0A8A0", 2.0, 2.0, 2);
}
