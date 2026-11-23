#include <hpq/VelledaTheme.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QFont>
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

TEST(VelledaTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::velleda_theme().palette();

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

TEST(VelledaTheme, PaletteIsAWhiteBoardWithABlackMarker) {
    const QPalette palette = hpq::velleda_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 245) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 50) << "role " << role;
    }
}

TEST(VelledaTheme, FramesAreBlueSelectionIsRedAndLinksAreGreen) {
    const QPalette palette = hpq::velleda_theme().palette();

    const QColor frame = palette.color(QPalette::Active, QPalette::Dark);
    EXPECT_GT(frame.blue(), frame.red());
    EXPECT_GT(frame.blue(), frame.green());
    const QColor selection = palette.color(QPalette::Active, QPalette::Highlight);
    EXPECT_GT(selection.red(), selection.green());
    EXPECT_GT(selection.red(), selection.blue());
    const QColor link = palette.color(QPalette::Active, QPalette::Link);
    EXPECT_GT(link.green(), link.red());
    EXPECT_GT(link.green(), link.blue());
}

TEST(VelledaTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::velleda_theme().palette();

    const double active_contrast = contrast(palette, QPalette::Active, QPalette::WindowText, QPalette::Window);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(contrast(palette, QPalette::Disabled, QPalette::WindowText, QPalette::Window), active_contrast);
}

TEST(VelledaTheme, SelectedTextIsReadableOnASolidSelection) {
    const QPalette palette = hpq::velleda_theme().palette();

    EXPECT_GE(contrast(palette, QPalette::Active, QPalette::HighlightedText, QPalette::Highlight), 4.5);
}

// The board shows between the hachures: the text of a selection is read on the board.
TEST(VelledaTheme, HatchedSelectionHasBlackTextUnderLightRedHachures) {
    const QPalette solid = hpq::velleda_theme().palette();
    const QPalette hatched = hpq::velleda_theme(hpq::FillStyle::Hachure).palette();

    for (const QPalette::ColorGroup group : Groups) {
        EXPECT_EQ(hatched.color(group, QPalette::HighlightedText), hatched.color(group, QPalette::Text))
            << "group " << group;
        EXPECT_GT(hatched.color(group, QPalette::Highlight).lightness(),
                  solid.color(group, QPalette::Highlight).lightness())
            << "group " << group;
    }
    EXPECT_GE(contrast(hatched, QPalette::Active, QPalette::HighlightedText, QPalette::Window), 4.5);
}

TEST(VelledaTheme, DrawsAThickLineOfCloseStrokesWithoutGrain) {
    const hpq::Theme theme = hpq::velleda_theme();
    const hpq::RoughOptions options = theme.rough_options();

    EXPECT_EQ(theme.medium(), hpq::Medium::Pencil);
    EXPECT_GE(options.stroke_count, 2);
    // The strokes stay within a pixel of the shape: they read as one line.
    EXPECT_LE(options.roughness * options.max_offset, 1.0);
    EXPECT_GT(options.hachure_gap, hpq::RoughOptions{}.hachure_gap);
}

TEST(VelledaTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::velleda_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::velleda_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
}

TEST(VelledaTheme, FontIsBoldArchitectsDaughter) {
    const QFont font = hpq::velleda_theme().font();

    EXPECT_EQ(font.family(), FontFamily);
    EXPECT_EQ(font.weight(), QFont::Bold);
}
