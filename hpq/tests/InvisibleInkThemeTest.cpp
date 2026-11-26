#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QListWidget>
#include <QPalette>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto FontFamily = "Architects Daughter";

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};
constexpr hpq::FillStyle FillStyles[] = {hpq::FillStyle::Solid, hpq::FillStyle::Hachure};
// The roles Qt writes a text with, the one of a selection aside.
constexpr QPalette::ColorRole OtherTextRoles[] = {
    QPalette::WindowText, QPalette::Text,        QPalette::ButtonText,  QPalette::BrightText,
    QPalette::ToolTipText, QPalette::PlaceholderText, QPalette::Link,   QPalette::LinkVisited};

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

// Whether every pixel of `area` of `image` has `color`.
bool is_plain(const QImage &image, const QRect &area, const QColor &color) {
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (image.pixelColor(x, y) != color) {
                return false;
            }
        }
    }
    return true;
}

// The thin lines of a text are smoothed: none of its pixels need have its exact color.
constexpr int TextColorTolerance = 40;

bool is_close(const QColor &first, const QColor &second) {
    return std::abs(first.red() - second.red()) <= TextColorTolerance &&
           std::abs(first.green() - second.green()) <= TextColorTolerance &&
           std::abs(first.blue() - second.blue()) <= TextColorTolerance;
}

// Whether a pixel of `area` of `image` is close to `color`.
bool has_pixel_close_to(const QImage &image, const QRect &area, const QColor &color) {
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (is_close(image.pixelColor(x, y), color)) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

TEST(InvisibleInkTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::invisible_ink_theme().palette;

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

TEST(InvisibleInkTheme, WritesWithAnInvisibleInkAndDrawsWithAPencil) {
    const hpq::Theme theme = hpq::invisible_ink_theme();

    EXPECT_EQ(theme.text_ink, hpq::TextInk::Invisible);
    EXPECT_EQ(theme.medium, hpq::Medium::Pencil);
}

TEST(InvisibleInkTheme, PaletteIsLightPaper) {
    const QPalette palette = hpq::invisible_ink_theme().palette;

    for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button}) {
        EXPECT_GT(palette.color(QPalette::Active, role).lightness(), 200) << "role " << role;
    }
}

// The arrows, the check marks and the cursor of a text have the color of a text: the pencil
// draws them, and they show on the paper.
TEST(InvisibleInkTheme, MarksInTheColorOfATextShowOnThePaper) {
    const QPalette palette = hpq::invisible_ink_theme().palette;

    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        const double active_contrast =
            contrast_ratio(palette.color(QPalette::Active, role), palette.color(QPalette::Active, QPalette::Window));
        EXPECT_GE(active_contrast, 4.5) << "role " << role;
        EXPECT_LT(contrast_ratio(palette.color(QPalette::Disabled, role),
                                 palette.color(QPalette::Disabled, QPalette::Window)),
                  active_contrast)
            << "role " << role;
    }
}

// The paint engine tells the text of a selection by its color: no other text may have it.
TEST(InvisibleInkTheme, NoOtherTextRoleHasTheColorOfTheTextOfASelection) {
    for (const hpq::FillStyle fill_style : FillStyles) {
        const QPalette palette = hpq::invisible_ink_theme(fill_style).palette;
        for (const QPalette::ColorGroup selection_group : Groups) {
            const QColor selection_text = palette.color(selection_group, QPalette::HighlightedText);
            for (const QPalette::ColorGroup group : Groups) {
                for (const QPalette::ColorRole role : OtherTextRoles) {
                    EXPECT_NE(palette.color(group, role), selection_text)
                        << "selection group " << selection_group << " group " << group << " role " << role;
                }
            }
        }
    }
}

// A solid selection is the violet light of the lamp: the ink glows on it.
TEST(InvisibleInkTheme, SolidSelectionIsADarkVioletWithLuminousText) {
    const QPalette palette = hpq::invisible_ink_theme().palette;

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor lamp = palette.color(group, QPalette::Highlight);
        const QColor text = palette.color(group, QPalette::HighlightedText);
        EXPECT_GT(lamp.blue(), lamp.red());
        EXPECT_GT(lamp.red(), lamp.green());
        EXPECT_LT(lamp.lightness(), 110);
        EXPECT_GT(text.lightness(), 180);
        EXPECT_GE(contrast_ratio(text, lamp), 4.5);
    }
}

// The paper shows between the hachures of a selection: its text must be readable on the paper.
TEST(InvisibleInkTheme, HachureVariantSelectionTextIsReadableOnPaper) {
    const QPalette palette = hpq::invisible_ink_theme(hpq::FillStyle::Hachure).palette;

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor text = palette.color(group, QPalette::HighlightedText);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Window)), 4.5);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Highlight)), 3.0);
    }
}

TEST(InvisibleInkTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::invisible_ink_theme().fill_style, hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::invisible_ink_theme(hpq::FillStyle::Hachure).fill_style, hpq::FillStyle::Hachure);
    EXPECT_NE(hpq::invisible_ink_theme().palette, hpq::invisible_ink_theme(hpq::FillStyle::Hachure).palette);
}

TEST(InvisibleInkTheme, FontIsArchitectsDaughter) {
    EXPECT_EQ(hpq::invisible_ink_theme().font.family(), FontFamily);
}

// A themed list of two items: the first one is selected.
class InvisibleInkListTest : public testing::Test {
protected:
    InvisibleInkListTest() : m_list(new QListWidget(&m_window)) {
        auto *layout = new QVBoxLayout(&m_window);
        layout->addWidget(m_list);
        m_list->addItems({"Selected", "Not selected"});
        // Letters large enough for their lines to be more than one pixel wide.
        QFont font;
        font.setPixelSize(TextPixelSize);
        m_list->setFont(font);
        m_window.resize(360, 200);
    }

    // The window as its theme draws it, with the first item selected.
    QImage grab_with_first_item_selected(const hpq::Theme &theme) {
        hpq::apply_theme(m_window, theme);
        m_list->setCurrentRow(0);
        m_window.show();
        QApplication::processEvents();
        return m_window.grab().toImage();
    }

    // Where the text of an item is, in the window: the inside of the item, away from the
    // edges of the list and of the selection, which wobble.
    QRect text_area(int row) const {
        const QRect item = m_list->visualItemRect(m_list->item(row)).adjusted(Margin, Margin, -Margin, -Margin);
        return QRect(m_list->viewport()->mapTo(&m_window, item.topLeft()), item.size());
    }

    static constexpr int Margin = 4;
    static constexpr int TextPixelSize = 32;

    QWidget m_window;
    QListWidget *m_list;
};

TEST_F(InvisibleInkListTest, TheTextOfAnItemThatIsNotSelectedDoesNotShow) {
    const hpq::Theme theme = hpq::invisible_ink_theme();

    const QImage image = grab_with_first_item_selected(theme);

    EXPECT_TRUE(is_plain(image, text_area(1), theme.palette.color(QPalette::Active, QPalette::Base)));
}

TEST_F(InvisibleInkListTest, TheTextOfASelectedItemShows) {
    for (const hpq::FillStyle fill_style : FillStyles) {
        const hpq::Theme theme = hpq::invisible_ink_theme(fill_style);

        const QImage image = grab_with_first_item_selected(theme);

        const QPalette palette = theme.palette;
        const QColor text = palette.color(QPalette::Active, QPalette::HighlightedText);
        // Neither the selection nor the paper could be taken for the text.
        ASSERT_FALSE(is_close(palette.color(QPalette::Active, QPalette::Highlight), text));
        ASSERT_FALSE(is_close(palette.color(QPalette::Active, QPalette::Base), text));
        EXPECT_TRUE(has_pixel_close_to(image, text_area(0), text)) << "fill style " << static_cast<int>(fill_style);
    }
}
