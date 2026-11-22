#include <hpq/HandPaintEffect.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QImage>
#include <QPalette>
#include <QWidget>

#include <cstdint>
#include <memory>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed);
}

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

    hpq::apply_theme(m_window, theme, rough_geometry_engine());

    EXPECT_TRUE(list().testAttribute(Qt::WA_SetPalette));
    EXPECT_EQ(list().palette(), theme.palette());
}

// A style that shows the list as a menu draws its entries with the application palette for
// every role that is not set on the list: a list that only inherits the palette stays light.
TEST_F(ComboBoxListThemeTest, ListIsDrawnInTheColorsOfTheTheme) {
    const hpq::Theme theme = hpq::slate_theme();

    hpq::apply_theme(m_window, theme, rough_geometry_engine());

    const QColor background = list_background();
    ASSERT_TRUE(background.isValid());
    EXPECT_EQ(background, theme.palette().color(QPalette::Active, QPalette::Window));
}

TEST_F(ComboBoxListThemeTest, ApplyThemeGivesTheWindowOfTheListItsOwnEffect) {
    hpq::apply_theme(m_window, hpq::slate_theme(), rough_geometry_engine());

    ASSERT_NE(list().window(), &m_window);
    EXPECT_TRUE(has_hand_paint_effect(*list().window()));
    EXPECT_TRUE(has_hand_paint_effect(m_window));
}

TEST_F(ComboBoxListThemeTest, AnotherThemeReplacesThePaletteOfTheList) {
    hpq::apply_theme(m_window, hpq::sketch_theme(), rough_geometry_engine());
    hpq::apply_theme(m_window, hpq::slate_theme(), rough_geometry_engine());

    EXPECT_EQ(list().palette(), hpq::slate_theme().palette());
    EXPECT_TRUE(has_hand_paint_effect(*list().window()));
}

TEST_F(ComboBoxListThemeTest, RemoveThemeTakesThePaletteAndTheEffectOffTheList) {
    const QColor background_before = list_background();
    ASSERT_TRUE(background_before.isValid());
    hpq::apply_theme(m_window, hpq::slate_theme(), rough_geometry_engine());

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
    hpq::apply_theme(m_window, hpq::slate_theme(), rough_geometry_engine());

    hpq::remove_theme(m_window);

    EXPECT_TRUE(list().testAttribute(Qt::WA_SetPalette));
    EXPECT_EQ(list().palette(), palette_before);
}
