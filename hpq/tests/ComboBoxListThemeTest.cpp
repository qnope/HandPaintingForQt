#include <hpq/HandPaintEffect.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QImage>
#include <QPalette>
#include <QWidget>

namespace {

// Where the color of a list is read: this far from its right and bottom edges, in pixels.
constexpr int RightMargin = 10;
constexpr int BottomMargin = 10;

bool has_hand_paint_effect(const QWidget &widget) {
    return qobject_cast<hpq::HandPaintEffect *>(widget.graphicsEffect()) != nullptr;
}

// A window with a combo box of three entries.
class ComboBoxListThemeTest : public testing::Test {
protected:
    ComboBoxListThemeTest() : m_combo_box(new QComboBox(&m_window)) {
        m_combo_box->addItems({"First", "Second", "Third"});
    }

    QAbstractItemView &list() { return *m_combo_box->view(); }

    // The color of the open list at the right of its last entry, where there is no text.
    // The current entry is the first one: the last one is not highlighted.
    // The list is drawn alone, without the effect of its window.
    QColor list_background() {
        m_window.show();
        m_combo_box->showPopup();
        QApplication::processEvents();
        const QImage image = list().grab().toImage();
        m_combo_box->hidePopup();
        return image.pixelColor(image.width() - RightMargin, image.height() - BottomMargin);
    }

    QWidget m_window;
    QComboBox *m_combo_box;
};

}  // namespace

TEST_F(ComboBoxListThemeTest, ApplyThemeSetsThePaletteOnTheList) {
    const hpq::Theme theme = hpq::slate_theme();

    hpq::apply_theme(m_window, theme);

    EXPECT_TRUE(list().testAttribute(Qt::WA_SetPalette));
    EXPECT_EQ(list().palette(), theme.palette);
}

// A style that shows the list as a menu draws its entries with the application palette for
// every role that is not set on the list: a list that only inherits the palette stays light.
TEST_F(ComboBoxListThemeTest, ListIsDrawnInTheColorsOfTheTheme) {
    const hpq::Theme theme = hpq::slate_theme();

    hpq::apply_theme(m_window, theme);

    const QColor background = list_background();
    ASSERT_TRUE(background.isValid());
    EXPECT_EQ(background, theme.palette.color(QPalette::Active, QPalette::Window));
}

TEST_F(ComboBoxListThemeTest, ApplyThemeGivesTheWindowOfTheListItsOwnEffect) {
    hpq::apply_theme(m_window, hpq::slate_theme());

    ASSERT_NE(list().window(), &m_window);
    EXPECT_TRUE(has_hand_paint_effect(*list().window()));
    EXPECT_TRUE(has_hand_paint_effect(m_window));
}

TEST_F(ComboBoxListThemeTest, AnotherThemeReplacesThePaletteOfTheList) {
    hpq::apply_theme(m_window, hpq::sketch_theme());
    hpq::apply_theme(m_window, hpq::slate_theme());

    EXPECT_EQ(list().palette(), hpq::slate_theme().palette);
    EXPECT_TRUE(has_hand_paint_effect(*list().window()));
}

TEST_F(ComboBoxListThemeTest, RemoveThemeTakesThePaletteAndTheEffectOffTheList) {
    const QColor background_before = list_background();
    ASSERT_TRUE(background_before.isValid());
    hpq::apply_theme(m_window, hpq::slate_theme());

    hpq::remove_theme(m_window);

    EXPECT_FALSE(list().testAttribute(Qt::WA_SetPalette));
    EXPECT_EQ(list().window()->graphicsEffect(), nullptr);
    EXPECT_EQ(list_background(), background_before);
}

TEST_F(ComboBoxListThemeTest, RemoveThemeGivesBackThePaletteSetOnTheList) {
    QPalette own_palette;
    own_palette.setColor(QPalette::Base, Qt::darkGreen);
    list().setPalette(own_palette);
    const QPalette palette_before = list().palette();
    hpq::apply_theme(m_window, hpq::slate_theme());

    hpq::remove_theme(m_window);

    EXPECT_TRUE(list().testAttribute(Qt::WA_SetPalette));
    EXPECT_EQ(list().palette(), palette_before);
}
