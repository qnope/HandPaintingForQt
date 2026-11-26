#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QPalette>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto FontFamily = "Architects Daughter";

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

// Relative luminance of a color, as defined by WCAG 2.
double relative_luminance(const QColor &color) {
    const auto linear = [](double channel) {
        return channel <= 0.03928 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

// WCAG 2 contrast ratio between two colors, from 1 to 21.
double contrast_ratio(const QColor &first, const QColor &second) {
    const double first_luminance = relative_luminance(first);
    const double second_luminance = relative_luminance(second);
    return (std::max(first_luminance, second_luminance) + 0.05) /
           (std::min(first_luminance, second_luminance) + 0.05);
}

double text_contrast(const QPalette &palette, QPalette::ColorGroup group) {
    return contrast_ratio(palette.color(group, QPalette::WindowText), palette.color(group, QPalette::Window));
}

}  // namespace

TEST(SlateTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::slate_theme().palette;

    for (const QPalette::ColorGroup group : Groups) {
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            // NoRole is not a color: Qt ignores it in the resolve mask, so isBrushSet() is always false for it.
            if (role == QPalette::NoRole) {
                continue;
            }
            EXPECT_TRUE(palette.isBrushSet(group, static_cast<QPalette::ColorRole>(role)))
                << "group " << group << " role " << role;
        }
    }
}

TEST(SlateTheme, PaletteIsDarkSlateWithLightChalk) {
    const QPalette palette = hpq::slate_theme().palette;

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 60) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 200) << "role " << role;
    }
}

TEST(SlateTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::slate_theme().palette;

    const double active_contrast = text_contrast(palette, QPalette::Active);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(text_contrast(palette, QPalette::Disabled), active_contrast);
}

TEST(SlateTheme, DrawsOneStrokeWithChalk) {
    const hpq::Theme theme = hpq::slate_theme();

    EXPECT_EQ(theme.medium, hpq::Medium::Chalk);
    EXPECT_EQ(theme.rough_options.stroke_count, 1);
}

TEST(SlateTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::slate_theme().fill_style, hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::slate_theme(hpq::FillStyle::Hachure).fill_style, hpq::FillStyle::Hachure);
    EXPECT_EQ(hpq::slate_theme(hpq::FillStyle::Hachure).medium, hpq::Medium::Chalk);
}

TEST(SlateTheme, FontIsArchitectsDaughter) {
    const hpq::Theme theme = hpq::slate_theme();

    EXPECT_EQ(theme.font.family(), FontFamily);
}

TEST(SketchTheme, DrawsWithAPencil) {
    EXPECT_EQ(hpq::sketch_theme().medium, hpq::Medium::Pencil);
}
