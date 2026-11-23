#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QFont>
#include <QFontDatabase>
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

double contrast(const QPalette &palette, QPalette::ColorGroup group, QPalette::ColorRole text,
                QPalette::ColorRole background) {
    return contrast_ratio(palette.color(group, text), palette.color(group, background));
}

}  // namespace

TEST(WatercolorTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::watercolor_theme().palette();

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

TEST(WatercolorTheme, PaletteIsDarkPaintOnAWhitePaper) {
    const QPalette palette = hpq::watercolor_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 235) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 70) << "role " << role;
    }
}

TEST(WatercolorTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::watercolor_theme().palette();

    const double active_contrast = contrast(palette, QPalette::Active, QPalette::WindowText, QPalette::Window);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(contrast(palette, QPalette::Disabled, QPalette::WindowText, QPalette::Window), active_contrast);
}

// A wash never hides what is under it, and nothing light can be painted over it: the text of
// a selection is the dark paint of any other text, on a wash light enough to read it.
TEST(WatercolorTheme, SelectionIsALightWashOfBlueUnderTheDarkPaintOfTheText) {
    for (const hpq::FillStyle fill_style : {hpq::FillStyle::Solid, hpq::FillStyle::Hachure}) {
        const QPalette palette = hpq::watercolor_theme(fill_style).palette();

        const QColor selection = palette.color(QPalette::Active, QPalette::Highlight);
        EXPECT_GT(selection.blue(), selection.red());
        EXPECT_GT(selection.blue(), selection.green());
        for (const QPalette::ColorGroup group : Groups) {
            EXPECT_EQ(palette.color(group, QPalette::HighlightedText), palette.color(group, QPalette::Text))
                << "group " << group;
        }
        EXPECT_GE(contrast(palette, QPalette::Active, QPalette::HighlightedText, QPalette::Highlight), 4.5);
        EXPECT_GE(contrast(palette, QPalette::Active, QPalette::HighlightedText, QPalette::Window), 4.5);
    }
}

// Under the darken composition of a wash, a color that is lighter than the paper in a channel
// would not show as it is.
TEST(WatercolorTheme, WashesAreNotLighterThanThePaperInAnyChannel) {
    const QPalette palette = hpq::watercolor_theme().palette();

    for (const QPalette::ColorGroup group : Groups) {
        const QColor paper = palette.color(group, QPalette::Window);
        for (const QPalette::ColorRole role : {QPalette::AlternateBase, QPalette::Midlight, QPalette::Highlight}) {
            const QColor wash = palette.color(group, role);
            EXPECT_LE(wash.red(), paper.red()) << "group " << group << " role " << role;
            EXPECT_LE(wash.green(), paper.green()) << "group " << group << " role " << role;
            EXPECT_LE(wash.blue(), paper.blue()) << "group " << group << " role " << role;
        }
    }
}

TEST(WatercolorTheme, TextIsReadableOnTheWashOfAnAlternateRow) {
    const QPalette palette = hpq::watercolor_theme().palette();

    EXPECT_GE(contrast(palette, QPalette::Active, QPalette::Text, QPalette::AlternateBase), 4.5);
}

TEST(WatercolorTheme, DrawsWithWatercolorInOneStroke) {
    const hpq::Theme theme = hpq::watercolor_theme();

    EXPECT_EQ(theme.medium(), hpq::Medium::Watercolor);
    EXPECT_EQ(theme.rough_options().stroke_count, 1);
}

TEST(WatercolorTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::watercolor_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::watercolor_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
    EXPECT_EQ(hpq::watercolor_theme(hpq::FillStyle::Hachure).medium(), hpq::Medium::Watercolor);
}

TEST(WatercolorTheme, FontIsArchitectsDaughterRegisteredInFontDatabase) {
    const hpq::Theme theme = hpq::watercolor_theme();

    EXPECT_EQ(theme.font().family(), FontFamily);
    EXPECT_TRUE(QFontDatabase::families().contains(FontFamily));
}
