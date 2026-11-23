#include <hpq/HighlighterTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QFont>
#include <QImage>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

constexpr auto FontFamily = "Architects Daughter";

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};
// The roles a highlighter fills.
constexpr QPalette::ColorRole FillRoles[] = {QPalette::Highlight, QPalette::Button, QPalette::Midlight,
                                             QPalette::AlternateBase};

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

// `color` as it shows when it is laid on `ground`, which is opaque.
QColor laid_on(const QColor &color, const QColor &ground) {
    const double opacity = color.alphaF();
    const auto mixed = [opacity](int over, int under) { return qRound(over * opacity + under * (1.0 - opacity)); };
    return QColor(mixed(color.red(), ground.red()), mixed(color.green(), ground.green()),
                  mixed(color.blue(), ground.blue()));
}

// The color of `role` as it shows on the paper.
QColor on_paper(const QPalette &palette, QPalette::ColorGroup group, QPalette::ColorRole role) {
    return laid_on(palette.color(group, role), palette.color(group, QPalette::Window));
}

bool is_fluorescent(const QColor &color) {
    return color.saturation() >= 180 && color.value() >= 230;
}

}  // namespace

TEST(HighlighterTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::highlighter_theme().palette();

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

TEST(HighlighterTheme, PaletteIsWhitePaperWithABlackPen) {
    const QPalette palette = hpq::highlighter_theme().palette();

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 245) << "role " << role;
        EXPECT_EQ(palette.color(QPalette::Active, role).alpha(), 255) << "role " << role;
    }
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        EXPECT_LT(palette.color(QPalette::Active, role).lightness(), 50) << "role " << role;
    }
}

TEST(HighlighterTheme, SelectionIsYellowButtonsAreGreenFramesArePinkAndAlternateRowsAreBlue) {
    const QPalette palette = hpq::highlighter_theme().palette();

    const QColor selection = palette.color(QPalette::Active, QPalette::Highlight);
    EXPECT_GT(std::min(selection.red(), selection.green()), selection.blue() + 100);
    const QColor button = palette.color(QPalette::Active, QPalette::Button);
    EXPECT_GT(button.green(), std::max(button.red(), button.blue()) + 100);
    const QColor frame = palette.color(QPalette::Active, QPalette::Dark);
    EXPECT_GT(frame.red(), frame.blue());
    EXPECT_GT(frame.blue(), frame.green() + 50);
    const QColor alternate_row = palette.color(QPalette::Active, QPalette::AlternateBase);
    EXPECT_GT(alternate_row.blue(), alternate_row.green());
    EXPECT_GT(alternate_row.green(), alternate_row.red() + 100);
}

TEST(HighlighterTheme, HighlightersAreFluorescent) {
    for (const hpq::FillStyle fill_style : {hpq::FillStyle::Solid, hpq::FillStyle::Hachure}) {
        const QPalette palette = hpq::highlighter_theme(fill_style).palette();

        for (const QPalette::ColorRole role : {QPalette::Highlight, QPalette::Button, QPalette::Midlight, QPalette::Dark}) {
            EXPECT_TRUE(is_fluorescent(palette.color(QPalette::Active, role))) << "role " << role;
        }
    }
}

// A highlighter does not hide what it marks.
TEST(HighlighterTheme, SolidHighlightersLetWhatIsUnderThemShow) {
    const QPalette palette = hpq::highlighter_theme().palette();

    for (const QPalette::ColorGroup group : Groups) {
        for (const QPalette::ColorRole role : FillRoles) {
            EXPECT_LT(palette.color(group, role).alpha(), 255) << "group " << group << " role " << role;
        }
        EXPECT_LT(palette.color(group, QPalette::Dark).alpha(), 255) << "group " << group;
    }
}

// A pressed button gets a second pass of the highlighter of the buttons.
TEST(HighlighterTheme, PressedButtonIsTheButtonHighlightedTwice) {
    const QPalette palette = hpq::highlighter_theme().palette();

    const QColor button = palette.color(QPalette::Active, QPalette::Button);
    const QColor pressed = palette.color(QPalette::Active, QPalette::Midlight);
    EXPECT_EQ(pressed.rgb(), button.rgb());
    EXPECT_GT(pressed.alpha(), button.alpha());
}

// An alternate row is only tinted: it stays close to the paper.
TEST(HighlighterTheme, AlternateRowIsALightPass) {
    const QPalette palette = hpq::highlighter_theme().palette();

    EXPECT_LT(palette.color(QPalette::Active, QPalette::AlternateBase).alpha(),
              palette.color(QPalette::Active, QPalette::Button).alpha());
    EXPECT_LT(contrast_ratio(on_paper(palette, QPalette::Active, QPalette::AlternateBase),
                             palette.color(QPalette::Active, QPalette::Window)),
              1.3);
}

TEST(HighlighterTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::highlighter_theme().palette();

    const double active_contrast = contrast_ratio(palette.color(QPalette::Active, QPalette::WindowText),
                                                  palette.color(QPalette::Active, QPalette::Window));
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(contrast_ratio(palette.color(QPalette::Disabled, QPalette::WindowText),
                             palette.color(QPalette::Disabled, QPalette::Window)),
              active_contrast);
}

// The text stays the one of the pen under a highlighter, and is read through it.
TEST(HighlighterTheme, TextIsReadableUnderTheHighlighters) {
    const QPalette palette = hpq::highlighter_theme().palette();

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        EXPECT_EQ(palette.color(group, QPalette::HighlightedText), palette.color(group, QPalette::Text));
        EXPECT_GE(contrast_ratio(palette.color(group, QPalette::HighlightedText),
                                 on_paper(palette, group, QPalette::Highlight)),
                  4.5);
        for (const QPalette::ColorRole role : {QPalette::Button, QPalette::Midlight}) {
            EXPECT_GE(contrast_ratio(palette.color(group, QPalette::ButtonText), on_paper(palette, group, role)), 4.5)
                << "role " << role;
        }
        EXPECT_GE(contrast_ratio(palette.color(group, QPalette::Text), on_paper(palette, group, QPalette::AlternateBase)),
                  4.5);
    }
}

TEST(HighlighterTheme, LinkIsReadableOnThePaper) {
    const QPalette palette = hpq::highlighter_theme().palette();

    EXPECT_GE(contrast_ratio(palette.color(QPalette::Active, QPalette::Link),
                             palette.color(QPalette::Active, QPalette::Window)),
              4.5);
}

// A disabled widget is marked by highlighters that have run dry.
TEST(HighlighterTheme, DisabledHighlightersArePaler) {
    for (const hpq::FillStyle fill_style : {hpq::FillStyle::Solid, hpq::FillStyle::Hachure}) {
        const QPalette palette = hpq::highlighter_theme(fill_style).palette();

        for (const QPalette::ColorRole role : {QPalette::Highlight, QPalette::Button, QPalette::Dark}) {
            EXPECT_LT(palette.color(QPalette::Disabled, role).saturation(),
                      palette.color(QPalette::Active, role).saturation())
                << "role " << role;
        }
    }
}

// The paint engine hatches only an opaque fill.
TEST(HighlighterTheme, HachureVariantFillsAreOpaque) {
    const QPalette palette = hpq::highlighter_theme(hpq::FillStyle::Hachure).palette();

    for (const QPalette::ColorGroup group : Groups) {
        for (const QPalette::ColorRole role : FillRoles) {
            EXPECT_EQ(palette.color(group, role).alpha(), 255) << "group " << group << " role " << role;
            EXPECT_NE(palette.color(group, role), palette.color(group, QPalette::Window))
                << "group " << group << " role " << role;
        }
    }
}

// The thin hachures of a yellow highlighter would not show on white paper: an orange one
// marks the selection, and its text is read on the paper between the hachures.
TEST(HighlighterTheme, HachureVariantSelectionIsOrangeAndShowsOnThePaper) {
    const QPalette solid = hpq::highlighter_theme().palette();
    const QPalette hatched = hpq::highlighter_theme(hpq::FillStyle::Hachure).palette();

    const QColor selection = hatched.color(QPalette::Active, QPalette::Highlight);
    EXPECT_GT(selection.red(), selection.green() + 50);
    EXPECT_GT(selection.green(), selection.blue() + 50);
    const QColor paper = hatched.color(QPalette::Active, QPalette::Window);
    EXPECT_GE(contrast_ratio(selection, paper), 1.8);
    EXPECT_GT(contrast_ratio(selection, paper),
              contrast_ratio(on_paper(solid, QPalette::Active, QPalette::Highlight), paper));
    EXPECT_GE(contrast_ratio(hatched.color(QPalette::Active, QPalette::HighlightedText), paper), 4.5);
}

TEST(HighlighterTheme, TwoPalettesOfTheThemeAreEqual) {
    EXPECT_EQ(hpq::highlighter_theme().palette(), hpq::highlighter_theme().palette());
    EXPECT_NE(hpq::highlighter_theme().palette(), hpq::highlighter_theme(hpq::FillStyle::Hachure).palette());
}

TEST(HighlighterTheme, DrawsALineOfCloseStrokesWithoutGrain) {
    const hpq::Theme theme = hpq::highlighter_theme();
    const hpq::RoughOptions options = theme.rough_options();

    EXPECT_EQ(theme.medium(), hpq::Medium::Pencil);
    EXPECT_GE(options.stroke_count, 3);
    EXPECT_LT(options.roughness * options.max_offset, hpq::RoughOptions{}.max_offset);
}

TEST(HighlighterTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::highlighter_theme().fill_style(), hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::highlighter_theme(hpq::FillStyle::Hachure).fill_style(), hpq::FillStyle::Hachure);
}

TEST(HighlighterTheme, FontIsArchitectsDaughter) {
    EXPECT_EQ(hpq::highlighter_theme().font().family(), FontFamily);
}

// The window of a themed widget is the paper, and its button is highlighted in green.
TEST(HighlighterTheme, ThemedButtonIsHighlightedOnThePaper) {
    const hpq::Theme theme = hpq::highlighter_theme();
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(40, 40, 40, 40);
    auto *button = new QPushButton;
    layout->addWidget(button);
    window.resize(240, 140);

    hpq::apply_theme(window, theme, std::make_shared<const hpq::RoughGeometryEngine>());
    const QImage grab = window.grab().toImage();

    ASSERT_EQ(grab.size(), window.size());
    EXPECT_EQ(grab.pixelColor(10, 10), theme.palette().color(QPalette::Active, QPalette::Window));
    const QColor inside = grab.pixelColor(button->geometry().center());
    const QColor expected = on_paper(theme.palette(), QPalette::Active, QPalette::Button);
    // The blend of the paint engine may round a channel another way.
    EXPECT_NEAR(inside.red(), expected.red(), 2);
    EXPECT_NEAR(inside.green(), expected.green(), 2);
    EXPECT_NEAR(inside.blue(), expected.blue(), 2);
    EXPECT_GT(inside.green(), std::max(inside.red(), inside.blue()) + 50);
}

// The text of a label is marked with the yellow highlighter, whose line is not a fill: it is
// the same with hachures.
TEST(HighlighterTheme, LabelMarkerIsATranslucentYellowHighlighter) {
    const QBrush marker = hpq::highlighter_theme().label_marker();

    ASSERT_EQ(marker.style(), Qt::SolidPattern);
    const QColor color = marker.color();
    EXPECT_TRUE(is_fluorescent(color));
    EXPECT_GT(std::min(color.red(), color.green()), color.blue() + 100);
    EXPECT_LT(color.alpha(), 255);
    EXPECT_EQ(hpq::highlighter_theme(hpq::FillStyle::Hachure).label_marker(), marker);
}

TEST(HighlighterTheme, LabelTextIsReadableOnItsMarker) {
    const hpq::Theme theme = hpq::highlighter_theme();
    const QColor paper = theme.palette().color(QPalette::Active, QPalette::Window);
    // Where every stroke of the line overlaps, the marker is the most vivid.
    QColor marked = paper;
    for (int stroke = 0; stroke < theme.rough_options().stroke_count; ++stroke) {
        marked = laid_on(theme.label_marker().color(), marked);
    }

    EXPECT_GE(contrast_ratio(theme.palette().color(QPalette::Active, QPalette::WindowText), marked), 4.5);
    EXPECT_GT(contrast_ratio(marked, paper), 1.1);
}

// The label of a themed window is marked in yellow on the paper.
TEST(HighlighterTheme, ThemedLabelIsMarkedInYellow) {
    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(40, 40, 40, 40);
    auto *label = new QLabel("A label");
    layout->addWidget(label, 0, Qt::AlignLeft | Qt::AlignTop);
    window.resize(240, 140);

    hpq::apply_theme(window, hpq::highlighter_theme(), std::make_shared<const hpq::RoughGeometryEngine>());
    const QImage grab = window.grab().toImage();

    const QRect area = label->geometry();
    int yellow_count = 0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            const QColor pixel = grab.pixelColor(x, y);
            if (pixel.red() > 200 && pixel.green() > 200 && pixel.blue() < 160) {
                ++yellow_count;
            }
        }
    }
    EXPECT_GT(yellow_count, area.width() * area.height() / 3);
}
