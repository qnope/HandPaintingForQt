#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>
#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>

#include <cstdint>
#include <memory>

// When a highlight moves, Qt repaints only the rectangles of the rows it leaves and reaches.
// What the theme drew past those rectangles must not stay on the screen: after the moves, the
// window must show what a repaint of the whole window shows.

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;
constexpr int EntryCount = 6;
// The rows the highlight goes to, one after the other: down the list, then by jumps. It does
// not end with a jump over the whole list, which would repaint all the rows between.
constexpr int HighlightedRows[] = {0, 1, 2, 3, 4, 5, 3, 1, 4, 2, 3};

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed);
}

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

class MovingHighlightTest : public testing::TestWithParam<hpq::Theme (*)(hpq::FillStyle)> {
protected:
    void apply_theme() { hpq::apply_theme(m_window, GetParam()(hpq::FillStyle::Solid), rough_geometry_engine()); }

    QWidget m_window;
};

}  // namespace

TEST_P(MovingHighlightTest, ComboBoxListLeavesNoTraceOfTheHighlight) {
    auto *combo_box = new QComboBox(&m_window);
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

TEST_P(MovingHighlightTest, TreeLeavesNoTraceOfTheHighlight) {
    auto *tree = new QTreeWidget(&m_window);
    tree->setGeometry(10, 10, 400, 190);
    for (int node = 0; node < EntryCount; ++node) {
        new QTreeWidgetItem(tree, QStringList{QString("Node %1").arg(node)});
    }
    apply_theme();
    m_window.show();

    move_highlight(*tree);

    EXPECT_EQ(stale_pixel_count(m_window), 0);
}

TEST_P(MovingHighlightTest, ScrollBarLeavesNoTraceOfItsHandle) {
    auto *table = new QTableWidget(3 * EntryCount, 3, &m_window);
    table->setGeometry(10, 10, 300, 160);
    apply_theme();
    m_window.show();
    QScrollBar &scroll_bar = *table->verticalScrollBar();

    for (const int row : HighlightedRows) {
        scroll_bar.setValue(row);
        shown(m_window);
    }

    EXPECT_EQ(stale_pixel_count(m_window), 0);
}

INSTANTIATE_TEST_SUITE_P(Themes, MovingHighlightTest, testing::Values(&hpq::sketch_theme, &hpq::watercolor_theme));
