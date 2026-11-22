#include <hpq/SlateTheme.h>

#include <gtest/gtest.h>

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QPalette>

#include <algorithm>
#include <cmath>
#include <cstdlib>

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

// The part of the pixels of `image` that are not transparent.
double coverage(const QImage &image) {
    int covered = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            covered += qAlpha(image.pixel(x, y)) > 127 ? 1 : 0;
        }
    }
    return static_cast<double>(covered) / (image.width() * image.height());
}

}  // namespace

TEST(SlateTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::slate_theme().palette();

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
    const QPalette palette = hpq::slate_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 60) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 200) << "role " << role;
    }
}

TEST(SlateTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::slate_theme().palette();

    const double active_contrast = text_contrast(palette, QPalette::Active);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(text_contrast(palette, QPalette::Disabled), active_contrast);
}

TEST(SlateTheme, EveryRoleIsATexturedBrush) {
    const QPalette palette = hpq::slate_theme().palette();

    for (const QPalette::ColorGroup group : Groups) {
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            if (role == QPalette::NoRole) {
                continue;
            }
            EXPECT_EQ(palette.brush(group, static_cast<QPalette::ColorRole>(role)).style(), Qt::TexturePattern)
                << "group " << group << " role " << role;
        }
    }
}

TEST(SlateTheme, SlateCoversEveryPixel) {
    const QPalette palette = hpq::slate_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_EQ(coverage(palette.brush(QPalette::Active, role).textureImage()), 1.0) << "role " << role;
    }
}

TEST(SlateTheme, ChalkOfTextIsDenserThanChalkOfOutlinesAndFills) {
    const QPalette palette = hpq::slate_theme().palette();

    const double text = coverage(palette.brush(QPalette::Active, QPalette::WindowText).textureImage());
    const double outline = coverage(palette.brush(QPalette::Active, QPalette::Dark).textureImage());
    const double fill = coverage(palette.brush(QPalette::Active, QPalette::Highlight).textureImage());

    EXPECT_LT(text, 1.0);
    EXPECT_GT(text, outline);
    EXPECT_GT(outline, fill);
    EXPECT_GT(fill, 0.3);
}

TEST(SlateTheme, CoveredPixelsOfABrushHaveItsColor) {
    const QBrush highlight = hpq::slate_theme().palette().brush(QPalette::Active, QPalette::Highlight);
    const QImage grain = highlight.textureImage().convertToFormat(QImage::Format_ARGB32);

    int checked = 0;
    for (int y = 0; y < grain.height(); ++y) {
        for (int x = 0; x < grain.width(); ++x) {
            if (qAlpha(grain.pixel(x, y)) == 255) {
                EXPECT_EQ(QColor(grain.pixel(x, y)), highlight.color());
                ++checked;
            }
        }
    }
    EXPECT_GT(checked, 0);
}

TEST(SlateTheme, GrainTilesWithoutASeam) {
    const QImage grain = hpq::slate_theme()
                             .palette()
                             .brush(QPalette::Active, QPalette::Window)
                             .textureImage()
                             .convertToFormat(QImage::Format_ARGB32);

    // The dust of the slate changes slowly: the last column is as close to the first column
    // as two neighbor columns are.
    const auto column_distance = [&](int first, int second) {
        int distance = 0;
        for (int y = 0; y < grain.height(); ++y) {
            distance += std::abs(qGray(grain.pixel(first, y)) - qGray(grain.pixel(second, y)));
        }
        return distance;
    };
    const int middle = grain.width() / 2;
    EXPECT_LE(column_distance(grain.width() - 1, 0), 2 * column_distance(middle, middle + 1) + grain.height());
}

TEST(SlateTheme, TwoThemesHaveTheSamePalette) {
    EXPECT_EQ(hpq::slate_theme().palette(), hpq::slate_theme().palette());
}

TEST(SlateTheme, DrawsTwoStrokesAndKeepsTheFillStyle) {
    EXPECT_EQ(hpq::slate_theme().rough_options().stroke_count, 2);
    EXPECT_EQ(hpq::slate_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::slate_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
}

TEST(SlateTheme, FontIsArchitectsDaughter) {
    const hpq::Theme theme = hpq::slate_theme();

    EXPECT_EQ(theme.font().family(), FontFamily);
}
