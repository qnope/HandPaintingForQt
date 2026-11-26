#include <hpq/HandPaintStyle.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QPixmap>
#include <QPointer>
#include <QScreen>
#include <QStyle>
#include <QStyleOptionMenuItem>
#include <QWidget>

// When a highlight moves, Qt repaints only the rectangles of the entries it leaves and reaches.
// What the theme drew past those rectangles must not stay on the screen: after the moves, the
// window must show what a repaint of the whole window shows.

namespace {

constexpr int EntryCount = 6;
// The rows the highlight goes to, one after the other: down the list, then by jumps. It does
// not end with a jump over the whole list, which would repaint all the rows between.
constexpr int HighlightedRows[] = {0, 1, 2, 3, 4, 5, 3, 1, 4, 2, 3};

// What the screen shows of `window`, once the pending repaints are done.
QImage shown(QWidget &window) {
    QApplication::processEvents();
    return window.screen()->grabWindow(window.winId()).toImage().convertToFormat(QImage::Format_RGB32);
}

// The number of pixels of `window` that a repaint of the whole window changes.
int stale_pixel_count(QWidget &window) {
    const QImage before = shown(window);
    window.repaint();
    const QImage after = shown(window);
    int count = 0;
    for (int y = 0; y < after.height(); ++y) {
        for (int x = 0; x < after.width(); ++x) {
            count += before.pixel(x, y) != after.pixel(x, y) ? 1 : 0;
        }
    }
    return count;
}

// Moves the highlight of `view` along HighlightedRows, the screen showing each move.
void move_highlight(QAbstractItemView &view) {
    for (const int row : HighlightedRows) {
        view.setCurrentIndex(view.model()->index(row, 0));
        shown(*view.window());
    }
}

// The theme is applied to a panel of the window, not to the window.
class MovingHighlightTest : public testing::TestWithParam<hpq::Theme (*)(hpq::FillStyle)> {
protected:
    MovingHighlightTest() : m_panel(new QWidget(&m_window)) { m_panel->setGeometry(0, 0, 400, 50); }

    void apply_theme() { hpq::apply_theme(*m_panel, GetParam()(hpq::FillStyle::Solid)); }

    QWidget m_window;
    QWidget *m_panel;
};

}  // namespace

TEST_P(MovingHighlightTest, ComboBoxListLeavesNoTraceOfTheHighlight) {
    auto *combo_box = new QComboBox(m_panel);
    combo_box->setGeometry(10, 10, 380, 30);
    for (int entry = 0; entry < EntryCount; ++entry) {
        combo_box->addItem(QString("Entry %1").arg(entry));
    }
    apply_theme();
    m_window.show();
    combo_box->showPopup();
    QAbstractItemView &list = *combo_box->view();

    move_highlight(list);

    EXPECT_EQ(stale_pixel_count(*list.window()), 0);
    combo_box->hidePopup();
}

INSTANTIATE_TEST_SUITE_P(Themes, MovingHighlightTest, testing::Values(&hpq::sketch_theme, &hpq::watercolor_theme));

// The height of an entry of a menu that `style` draws, for an entry whose contents are 10 pixels high.
int menu_entry_height(const QStyle &style) {
    QStyleOptionMenuItem option;
    option.menuItemType = QStyleOptionMenuItem::Normal;
    return style.sizeFromContents(QStyle::CT_MenuItem, &option, QSize(10, 10), nullptr).height();
}

// An entry is larger on each side by twice the largest offset of the theme, rounded up.
TEST(HighlightOverflow, EntryOfAMenuLeavesRoomForTheHighlightOfTheTheme) {
    const hpq::HandPaintStyle sketch_style(hpq::sketch_theme());
    const hpq::HandPaintStyle velleda_style(hpq::velleda_theme());

    // The largest offsets are 2 pixels for Sketch and 0.9 pixel for Velleda.
    EXPECT_EQ(menu_entry_height(sketch_style) - menu_entry_height(velleda_style), 2 * 4 - 2 * 2);
}

TEST(HighlightOverflow, ApplyThemeGivesTheWidgetAStyleOfItsTheme) {
    QWidget window;
    auto *panel = new QWidget(&window);
    const hpq::HandPaintStyle velleda_style(hpq::velleda_theme());

    hpq::apply_theme(*panel, hpq::velleda_theme());

    ASSERT_NE(qobject_cast<hpq::HandPaintStyle *>(panel->style()), nullptr);
    EXPECT_EQ(menu_entry_height(*panel->style()), menu_entry_height(velleda_style));
}

TEST(HighlightOverflow, AnotherThemeReplacesTheStyleAndRemoveThemeDeletesIt) {
    QWidget window;
    hpq::apply_theme(window, hpq::sketch_theme());
    const QPointer<QStyle> sketch_style = window.style();
    hpq::apply_theme(window, hpq::velleda_theme());
    const QPointer<QStyle> velleda_style = window.style();

    EXPECT_TRUE(sketch_style.isNull());
    ASSERT_NE(qobject_cast<hpq::HandPaintStyle *>(velleda_style.data()), nullptr);

    hpq::remove_theme(window);

    EXPECT_TRUE(velleda_style.isNull());
    EXPECT_EQ(qobject_cast<hpq::HandPaintStyle *>(window.style()), nullptr);
}

// The entries of a list already shown get the size the new theme gives them.
TEST(HighlightOverflow, AnotherThemeResizesTheEntriesOfAComboBoxList) {
    QWidget window;
    auto *combo_box = new QComboBox(&window);
    combo_box->addItems({"First", "Second"});
    QWidget other_window;
    auto *other_combo_box = new QComboBox(&other_window);
    other_combo_box->addItems({"First", "Second"});
    hpq::apply_theme(other_window, hpq::velleda_theme());
    hpq::apply_theme(window, hpq::watercolor_theme());
    window.show();
    combo_box->showPopup();
    QApplication::processEvents();
    combo_box->hidePopup();

    hpq::apply_theme(window, hpq::velleda_theme());

    QAbstractItemView &list = *combo_box->view();
    QAbstractItemView &other_list = *other_combo_box->view();
    EXPECT_EQ(list.sizeHintForRow(0), other_list.sizeHintForRow(0));
    EXPECT_EQ(list.visualRect(list.model()->index(0, 0)).height(), list.sizeHintForRow(0));
}
