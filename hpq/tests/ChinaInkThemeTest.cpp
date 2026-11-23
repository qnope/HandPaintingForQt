#include <hpq/ChinaInkTheme.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QFile>
#include <QFontDatabase>
#include <QPalette>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto FontFamily = "Kaushan Script";

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

TEST(ChinaInkTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::china_ink_theme().palette();

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

TEST(ChinaInkTheme, PaletteIsLightPaperWithBlackInk) {
    const QPalette palette = hpq::china_ink_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 200) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 40) << "role " << role;
    }
}

TEST(ChinaInkTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::china_ink_theme().palette();

    const double active_contrast = text_contrast(palette, QPalette::Active);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(text_contrast(palette, QPalette::Disabled), active_contrast);
}

// The selection has the red of a seal, and its text is read on it.
TEST(ChinaInkTheme, SelectionIsSealRedWithReadableText) {
    const QPalette palette = hpq::china_ink_theme().palette();

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor seal = palette.color(group, QPalette::Highlight);
        EXPECT_GT(seal.red(), 2 * seal.green());
        EXPECT_GT(seal.red(), 2 * seal.blue());
        EXPECT_GE(contrast_ratio(palette.color(group, QPalette::HighlightedText), seal), 4.5);
    }
}

// A brush draws a line in one stroke: a second one would hide its thick and thin.
TEST(ChinaInkTheme, DrawsOneStrokeWithABrush) {
    const hpq::Theme theme = hpq::china_ink_theme();

    EXPECT_EQ(theme.medium(), hpq::Medium::Ink);
    EXPECT_EQ(theme.rough_options().stroke_count, 1);
}

TEST(ChinaInkTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::china_ink_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::china_ink_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
    EXPECT_EQ(hpq::china_ink_theme(hpq::FillStyle::Hachure).medium(), hpq::Medium::Ink);
}

// The paper shows between the hachures of a selection: its text must be readable on the paper.
TEST(ChinaInkTheme, HachureVariantSelectionTextIsReadableOnPaper) {
    const QPalette palette = hpq::china_ink_theme(hpq::FillStyle::Hachure).palette();

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor text = palette.color(group, QPalette::HighlightedText);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Window)), 4.5);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Highlight)), 3.0);
    }
}

TEST(ChinaInkTheme, FontIsKaushanScript) {
    const hpq::Theme theme = hpq::china_ink_theme();

    EXPECT_EQ(theme.font().family(), FontFamily);
}

TEST(ChinaInkTheme, RegistersKaushanScriptInFontDatabase) {
    const hpq::Theme theme = hpq::china_ink_theme();

    EXPECT_TRUE(QFontDatabase::families().contains(FontFamily));
}

TEST(ChinaInkTheme, EmbedsFontAndItsLicenseAsResources) {
    const hpq::Theme theme = hpq::china_ink_theme();

    EXPECT_TRUE(QFile::exists(":/hpq/fonts/KaushanScript-Regular.ttf"));
    EXPECT_TRUE(QFile::exists(":/hpq/fonts/KaushanScript-OFL.txt"));
}
