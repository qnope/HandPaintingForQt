#include <hpq/BlueprintTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QImage>
#include <QPalette>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

constexpr auto FontFamily = "Architects Daughter";

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};
constexpr QPalette::ColorRole PaperRoles[] = {QPalette::Window, QPalette::Base, QPalette::Button};

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

bool is_blue(const QColor &color) {
    return color.blue() > color.green() && color.green() > color.red();
}

}  // namespace

TEST(BlueprintTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::blueprint_theme().palette();

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

TEST(BlueprintTheme, PaletteIsBluePaperWithLightInk) {
    const QPalette palette = hpq::blueprint_theme().palette();

    for (const QPalette::ColorRole role : PaperRoles) {
        const QColor paper = palette.color(QPalette::Active, role);
        EXPECT_TRUE(is_blue(paper)) << "role " << role;
        EXPECT_LT(paper.lightness(), 100) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 200) << "role " << role;
    }
}

TEST(BlueprintTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::blueprint_theme().palette();

    const double active_contrast = text_contrast(palette, QPalette::Active);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(text_contrast(palette, QPalette::Disabled), active_contrast);
}

// The grid is printed on the paper: its squares tile, and its lines are a lighter blue.
TEST(BlueprintTheme, PaperIsATextureOfAGridLighterThanThePaper) {
    const QPalette palette = hpq::blueprint_theme().palette();

    for (const QPalette::ColorGroup group : Groups) {
        for (const QPalette::ColorRole role : PaperRoles) {
            const QBrush paper = palette.brush(group, role);
            ASSERT_EQ(paper.style(), Qt::TexturePattern) << "group " << group << " role " << role;
            const QImage tile = paper.textureImage();
            ASSERT_GT(tile.width(), 2);
            EXPECT_EQ(tile.width(), tile.height());

            const QColor inside = tile.pixelColor(tile.width() / 2 + 1, tile.height() / 2 + 1);
            EXPECT_EQ(inside, paper.color());
            const QColor heavy_line = tile.pixelColor(0, tile.height() / 2 + 1);
            EXPECT_TRUE(is_blue(heavy_line));
            EXPECT_GT(heavy_line.lightness(), inside.lightness());
            // The lines are faint: far from the ink that draws on the paper.
            EXPECT_LT(contrast_ratio(heavy_line, inside), 1.6);
        }
    }
}

// Qt compares two textures by the image they share, not by their pixels.
TEST(BlueprintTheme, TwoPalettesOfTheThemeAreEqual) {
    EXPECT_EQ(hpq::blueprint_theme().palette(), hpq::blueprint_theme().palette());
    EXPECT_NE(hpq::blueprint_theme().palette(), hpq::blueprint_theme(hpq::FillStyle::Hachure).palette());
}

TEST(BlueprintTheme, DrawsOneStrokeWithAPencilCloserToTheLineThanTheDefault) {
    const hpq::Theme theme = hpq::blueprint_theme();

    EXPECT_EQ(theme.medium(), hpq::Medium::Pencil);
    EXPECT_EQ(theme.rough_options().stroke_count, 1);
    EXPECT_LT(theme.rough_options().roughness, hpq::RoughOptions{}.roughness);
    EXPECT_DOUBLE_EQ(theme.rough_options().hachure_angle, -45.0);
}

TEST(BlueprintTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::blueprint_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::blueprint_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
}

// A solid selection is a block of light ink: its text is in the blue of the paper.
TEST(BlueprintTheme, SolidSelectionTextIsReadableOnTheSelection) {
    const QPalette palette = hpq::blueprint_theme().palette();

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        EXPECT_GE(contrast_ratio(palette.color(group, QPalette::HighlightedText),
                                 palette.color(group, QPalette::Highlight)),
                  4.5);
    }
}

// The paper shows between the hachures of a selection: its text must be readable on the paper.
TEST(BlueprintTheme, HachureVariantSelectionTextIsReadableOnPaper) {
    const QPalette palette = hpq::blueprint_theme(hpq::FillStyle::Hachure).palette();

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor text = palette.color(group, QPalette::HighlightedText);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Window)), 4.5);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Highlight)), 3.0);
    }
}

TEST(BlueprintTheme, FontIsArchitectsDaughterInCapitals) {
    const hpq::Theme theme = hpq::blueprint_theme();

    EXPECT_EQ(theme.font().family(), FontFamily);
    EXPECT_EQ(theme.font().capitalization(), QFont::AllUppercase);
}

// The window of a themed widget is the paper: the paint engine draws its grid, unchanged.
TEST(BlueprintTheme, ThemedWindowShowsTheGridOfThePaper) {
    const hpq::Theme theme = hpq::blueprint_theme();
    const QImage tile = theme.palette().window().textureImage();
    QWidget window;
    window.resize(3 * tile.width(), 3 * tile.height());

    hpq::apply_theme(window, theme, std::make_shared<const hpq::RoughGeometryEngine>());
    const QImage grab = window.grab().toImage();

    ASSERT_EQ(grab.size(), window.size());
    // Away from the edges of the window, where the fill of its background wobbles.
    for (int y = tile.height(); y < 2 * tile.height(); ++y) {
        for (int x = tile.width(); x < 2 * tile.width(); ++x) {
            ASSERT_EQ(grab.pixelColor(x, y), tile.pixelColor(x % tile.width(), y % tile.height()))
                << "x " << x << " y " << y;
        }
    }
}
