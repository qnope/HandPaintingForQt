#include <hpq/HandPaintDevice.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QImage>
#include <QListView>
#include <QPainter>
#include <QPalette>
#include <QWidget>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto FontFamily = "Playwrite FR Moderne";

constexpr QPalette::ColorGroup Groups[] = {QPalette::Active, QPalette::Inactive, QPalette::Disabled};

// The ruling of the sheet, in pixels: the side of a square, the distance between two
// horizontal lines, and the distance from the left of the sheet to its margin line.
constexpr int Square = 32;
constexpr int Interline = 8;
constexpr int Margin = 64;

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

// The image that the window brush of the theme tiles.
QImage sheet() {
    return hpq::notebook_theme().palette.brush(QPalette::Active, QPalette::Window).textureImage();
}

// The color of the sheet at a point of a window that the sheet tiles from its top left corner.
QColor sheet_color(const QImage &sheet, int x, int y) {
    return sheet.pixelColor(x % sheet.width(), y % sheet.height());
}

}  // namespace

TEST(NotebookTheme, PaletteSetsEveryRoleOfEveryGroup) {
    const QPalette palette = hpq::notebook_theme().palette;

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

TEST(NotebookTheme, WindowIsASheetOfLightPaper) {
    const QPalette palette = hpq::notebook_theme().palette;

    for (const QPalette::ColorGroup group : Groups) {
        const QBrush window = palette.brush(group, QPalette::Window);
        EXPECT_EQ(window.style(), Qt::TexturePattern) << "group " << group;
        EXPECT_GT(window.color().lightness(), 240) << "group " << group;
    }
}

TEST(NotebookTheme, SheetIsOneSquareHighAndTilesItsHorizontalLines) {
    const QImage image = sheet();
    const QColor paper = hpq::notebook_theme().palette.color(QPalette::Active, QPalette::Window);
    // Between the left of the sheet and the margin line, where no vertical line is drawn.
    const int x = Margin / 2;

    ASSERT_EQ(image.height(), Square);
    const QColor line = image.pixelColor(x, 0);
    const QColor fine_line = image.pixelColor(x, Interline);
    EXPECT_NE(line, paper);
    EXPECT_NE(fine_line, paper);
    // The lines of the squares are heavier than the lines between them.
    EXPECT_LT(line.lightness(), fine_line.lightness());
    for (int y = 0; y < Square; ++y) {
        const QColor expected = y == 0 ? line : y % Interline == 0 ? fine_line : paper;
        EXPECT_EQ(image.pixelColor(x, y), expected) << "y " << y;
    }
}

TEST(NotebookTheme, SheetHasARedMarginAndVerticalLinesAtItsRightOnly) {
    const QImage image = sheet();
    const QColor paper = hpq::notebook_theme().palette.color(QPalette::Active, QPalette::Window);
    // Between two horizontal lines.
    const int y = Interline / 2;

    const QColor margin = image.pixelColor(Margin, y);
    EXPECT_GT(margin.red(), margin.blue());
    EXPECT_GT(margin.red(), margin.green());
    for (int x = 0; x < Margin; ++x) {
        EXPECT_EQ(image.pixelColor(x, y), paper) << "x " << x;
    }
    const QColor line = image.pixelColor(Margin + Square, y);
    EXPECT_NE(line, paper);
    EXPECT_GT(line.blue(), line.red());
    for (int x = Margin + 1; x < Margin + 4 * Square; ++x) {
        const QColor expected = (x - Margin) % Square == 0 ? line : paper;
        EXPECT_EQ(image.pixelColor(x, y), expected) << "x " << x;
    }
}

// Two brushes are equal only when they share their image: without one image for every theme,
// the palettes of two themes would differ, and a widget could not be told to have the theme.
TEST(NotebookTheme, TwoThemesHaveTheSamePalette) {
    EXPECT_EQ(hpq::notebook_theme().palette, hpq::notebook_theme().palette);
    EXPECT_NE(hpq::notebook_theme().palette, hpq::notebook_theme(hpq::FillStyle::Hachure).palette);
}

TEST(NotebookTheme, ViewsAndButtonsAreSeeThrough) {
    const QPalette palette = hpq::notebook_theme().palette;

    for (const QPalette::ColorGroup group : Groups) {
        EXPECT_EQ(palette.color(group, QPalette::Base).alpha(), 0) << "group " << group;
        EXPECT_EQ(palette.color(group, QPalette::Button).alpha(), 0) << "group " << group;
    }
}

TEST(NotebookTheme, TextIsBlueInk) {
    const QPalette palette = hpq::notebook_theme().palette;

    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        const QColor ink = palette.color(QPalette::Active, role);
        EXPECT_GT(ink.blue(), 2 * ink.red()) << "role " << role;
        EXPECT_GT(ink.blue(), 2 * ink.green()) << "role " << role;
    }
}

TEST(NotebookTheme, ActiveTextContrastIsReadableAndDisabledIsLower) {
    const QPalette palette = hpq::notebook_theme().palette;

    const double active_contrast = text_contrast(palette, QPalette::Active);
    EXPECT_GE(active_contrast, 4.5);
    EXPECT_LT(text_contrast(palette, QPalette::Disabled), active_contrast);
}

// What is disabled is written with a pencil: grey, without the blue of the ink.
TEST(NotebookTheme, DisabledTextIsPencilGrey) {
    const QColor pencil = hpq::notebook_theme().palette.color(QPalette::Disabled, QPalette::WindowText);

    EXPECT_LT(pencil.saturation(), 30);
}

// A selection is a stroke of highlighter: the ruling and the ink show through it.
TEST(NotebookTheme, SelectionIsATranslucentHighlighterUnderInk) {
    const QPalette palette = hpq::notebook_theme().palette;

    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor highlight = palette.color(group, QPalette::Highlight);
        EXPECT_LT(highlight.alpha(), 255) << "group " << group;
        EXPECT_GT(highlight.alpha(), 0) << "group " << group;
        EXPECT_EQ(palette.color(group, QPalette::HighlightedText), palette.color(group, QPalette::Text));
        EXPECT_GE(contrast_ratio(palette.color(group, QPalette::HighlightedText), highlight), 4.5);
    }
}

// Only an opaque brush is hatched, and the paper shows between the hachures.
TEST(NotebookTheme, HachureVariantSelectionIsOpaqueAndItsTextReadableOnPaper) {
    const QPalette palette = hpq::notebook_theme(hpq::FillStyle::Hachure).palette;

    for (const QPalette::ColorGroup group : Groups) {
        EXPECT_EQ(palette.color(group, QPalette::Highlight).alpha(), 255) << "group " << group;
    }
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        const QColor text = palette.color(group, QPalette::HighlightedText);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Window)), 4.5);
        EXPECT_GE(contrast_ratio(text, palette.color(group, QPalette::Highlight)), 3.0);
    }
}

TEST(NotebookTheme, KeepsTheFillStyleItIsGiven) {
    EXPECT_EQ(hpq::notebook_theme().fill_style, hpq::FillStyle::Solid);
    EXPECT_EQ(hpq::notebook_theme(hpq::FillStyle::Hachure).fill_style, hpq::FillStyle::Hachure);
}

TEST(NotebookTheme, DrawsOneStrokeWithAPen) {
    const hpq::Theme theme = hpq::notebook_theme();

    EXPECT_EQ(theme.medium, hpq::Medium::Pencil);
    EXPECT_EQ(theme.rough_options.stroke_count, 1);
}

TEST(NotebookTheme, FontIsPlaywriteFrModerne) {
    const hpq::Theme theme = hpq::notebook_theme();

    EXPECT_EQ(theme.font.family(), FontFamily);
    EXPECT_EQ(QFontInfo(theme.font).family(), FontFamily);
}

TEST(NotebookTheme, RegistersItsFontInFontDatabase) {
    const hpq::Theme theme = hpq::notebook_theme();

    EXPECT_TRUE(QFontDatabase::families().contains(FontFamily));
}

TEST(NotebookTheme, EmbedsFontAndItsLicenseAsResources) {
    const hpq::Theme theme = hpq::notebook_theme();

    EXPECT_TRUE(QFile::exists(":/hpq/fonts/PlaywriteFRModerne-Regular.ttf"));
    EXPECT_TRUE(QFile::exists(":/hpq/fonts/OFL-PlaywriteFRModerne.txt"));
}

// The handwriting has tall capitals and long ascenders: it is written smaller than the
// application font, so that a line of text does not get twice as high.
TEST(NotebookTheme, FontIsSmallerThanTheApplicationFont) {
    const QFont application_font = QApplication::font();
    const QFont font = hpq::notebook_theme().font;

    if (application_font.pointSizeF() > 0) {
        EXPECT_LT(font.pointSizeF(), application_font.pointSizeF());
        EXPECT_GT(font.pointSizeF(), application_font.pointSizeF() / 2);
    } else {
        EXPECT_LT(font.pixelSize(), application_font.pixelSize());
        EXPECT_GT(font.pixelSize(), application_font.pixelSize() / 2);
    }
}

// The base of a view is see-through: the sheet of the window is one page under every widget.
TEST(NotebookTheme, RulingOfTheWindowRunsUnderAView) {
    QWidget window;
    window.resize(400, 300);
    QListView view(&window);
    view.setGeometry(100, 40, 200, 200);
    hpq::apply_theme(window, hpq::notebook_theme());
    window.show();
    QApplication::processEvents();

    const QImage image = window.grab().toImage();

    const QImage ruling = sheet();
    ASSERT_DOUBLE_EQ(image.devicePixelRatio(), 1.0);
    // Well inside the view: the wobble of its frame does not reach these points.
    for (int y = 60; y < 220; y += 3) {
        for (int x = 120; x < 280; x += 7) {
            ASSERT_EQ(image.pixelColor(x, y), sheet_color(ruling, x, y)) << "x " << x << " y " << y;
        }
    }
}

// The paper drawn under the hachures of a fill is the sheet, with its ruling where the
// window has it.
TEST(NotebookTheme, RulingShowsBetweenTheHachuresOfAFill) {
    const hpq::Theme theme = hpq::notebook_theme(hpq::FillStyle::Hachure);
    QImage image(200, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);

    {
        QPainter target(&image);
        hpq::HandPaintDevice device(target, theme);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xc0, 0x39, 0x2b));
        painter.drawRect(20, 20, 160, 160);
    }

    const QImage ruling = sheet();
    // Along a line of the squares and along a line of paper, well inside the fill.
    for (const int y : {3 * Square, 3 * Square + Interline / 2}) {
        int sheet_pixels = 0;
        for (int x = 40; x < 160; ++x) {
            if (image.pixelColor(x, y) == sheet_color(ruling, x, y)) {
                ++sheet_pixels;
            }
        }
        // The hachures are thin lines: more of the row is paper than hachure.
        EXPECT_GT(sheet_pixels, 60) << "y " << y;
        EXPECT_LT(sheet_pixels, 120) << "y " << y;
    }
}
