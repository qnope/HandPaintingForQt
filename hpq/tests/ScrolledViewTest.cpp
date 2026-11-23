#include <hpq/GeometryEngine.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QHeaderView>
#include <QImage>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;
constexpr int RowCount = 60;
constexpr int ColumnCount = 6;
// Pixels by which a test scrolls a view: not a whole number of rows or columns.
constexpr int ScrollStep = 23;
// The shapes at the edges of a viewport are cut there: the compared part keeps away from them.
constexpr int EdgeMargin = 8;

// A rough geometry engine that counts the shapes it was asked to make rough.
class CountingGeometryEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &options,
                                      std::uint32_t seed) const override {
        ++m_call_count;
        return m_engine.outline(shape, options, seed);
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &options,
                           std::uint32_t seed) const override {
        ++m_call_count;
        return m_engine.fill_area(shape, options, seed);
    }

    QPainterPath hachure(const QPainterPath &shape, const hpq::RoughOptions &options,
                         std::uint32_t seed) const override {
        ++m_call_count;
        return m_engine.hachure(shape, options, seed);
    }

    int call_count() const { return m_call_count; }

private:
    hpq::RoughGeometryEngine m_engine{BaseSeed};
    mutable int m_call_count = 0;
};

// A window holding a table, themed with hachures, so that its rows are rough fills.
class ThemedTable {
public:
    explicit ThemedTable(std::shared_ptr<const hpq::GeometryEngine> geometry_engine =
                             std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed))
        : m_table(new QTableView(&m_window)) {
        auto *layout = new QVBoxLayout(&m_window);
        layout->addWidget(m_table);
        auto *model = new QStandardItemModel(RowCount, ColumnCount, m_table);
        for (int row = 0; row < RowCount; ++row) {
            for (int column = 0; column < ColumnCount; ++column) {
                model->setData(model->index(row, column), QString("%1-%2").arg(row).arg(column));
            }
        }
        m_table->setModel(model);
        m_table->setAlternatingRowColors(true);
        m_table->selectRow(3);
        m_window.resize(320, 260);
        hpq::apply_theme(m_window, hpq::sketch_theme(hpq::FillStyle::Hachure), std::move(geometry_engine));
        m_window.show();
        QCoreApplication::processEvents();
    }

    QTableView &table() { return *m_table; }

    // Scrolls the table by `offset` pixels, or by `offset` rows and columns when it scrolls
    // by items.
    void scroll_by(const QPoint &offset) {
        m_table->horizontalScrollBar()->setValue(m_table->horizontalScrollBar()->value() + offset.x());
        m_table->verticalScrollBar()->setValue(m_table->verticalScrollBar()->value() + offset.y());
        QCoreApplication::processEvents();
    }

    QImage window_image() { return m_window.grab().toImage(); }

    // What the viewport of the table shows, away from its edges.
    QImage viewport_image() {
        QWidget *viewport = m_table->viewport();
        const QRect area(viewport->mapTo(&m_window, QPoint(0, 0)), viewport->size());
        return window_image().copy(area.adjusted(EdgeMargin, EdgeMargin, -EdgeMargin, -EdgeMargin));
    }

private:
    QWidget m_window;
    QTableView *m_table;
};

// Whether `second` shows `first` moved by `offset`, where both show it.
bool shows_moved(const QImage &first, const QImage &second, const QPoint &offset) {
    const QRect common = first.rect().intersected(first.rect().translated(offset));
    return first.copy(common.translated(-offset)) == second.copy(common);
}

}  // namespace

TEST(ScrolledViewTest, ScrollingAwayAndBackGivesTheSameImage) {
    ThemedTable themed;
    const QImage before = themed.window_image();

    for (int step = 0; step < 4; ++step) {
        themed.scroll_by(QPoint(1, 3));
    }
    const QImage away = themed.window_image();
    themed.scroll_by(QPoint(-4, -12));
    const QImage back = themed.window_image();

    EXPECT_NE(before, away);
    EXPECT_EQ(before, back);
}

// The shapes of a view are kept by their place in its content: the ones that come back when
// it scrolls back are drawn again as they were, without being made rough again.
TEST(ScrolledViewTest, ScrollingBackMakesNoShapeAgain) {
    const auto engine = std::make_shared<const CountingGeometryEngine>();
    ThemedTable themed(engine);
    themed.table().setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    themed.table().setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    const QImage before = themed.window_image();
    ASSERT_GT(engine->call_count(), 0);

    themed.scroll_by(QPoint(ScrollStep, 3 * ScrollStep));
    const QImage away = themed.window_image();
    themed.scroll_by(-QPoint(ScrollStep, 3 * ScrollStep));
    const int calls_before_coming_back = engine->call_count();
    const QImage back = themed.window_image();

    EXPECT_NE(before, away);
    EXPECT_EQ(before, back);
    EXPECT_EQ(engine->call_count(), calls_before_coming_back);
}

// A window that is drawn again as it is makes no shape again.
TEST(ScrolledViewTest, DrawingAgainMakesNoShapeAgain) {
    const auto engine = std::make_shared<const CountingGeometryEngine>();
    ThemedTable themed(engine);
    const QImage first = themed.window_image();
    const int calls_of_first_drawing = engine->call_count();

    const QImage again = themed.window_image();

    EXPECT_EQ(first, again);
    EXPECT_EQ(engine->call_count(), calls_of_first_drawing);
}

TEST(ScrolledViewTest, ScrolledRowsKeepTheirDrawing) {
    ThemedTable themed;
    themed.table().setShowGrid(false);
    themed.table().setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    themed.table().setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    const QImage before = themed.viewport_image();

    themed.scroll_by(QPoint(ScrollStep, ScrollStep));
    const QImage after = themed.viewport_image();

    EXPECT_TRUE(shows_moved(before, after, -QPoint(ScrollStep, ScrollStep)));
}

// Rows of the same size at two places of the table are drawn by hand twice: their hachures
// do not wobble the same way.
TEST(ScrolledViewTest, RowsOfTheSameSizeAreDrawnDifferently) {
    ThemedTable themed;
    themed.table().setShowGrid(false);
    themed.table().setAlternatingRowColors(false);
    themed.table().clearSelection();
    auto &model = static_cast<QStandardItemModel &>(*themed.table().model());
    model.clear();
    model.setRowCount(RowCount);
    model.setColumnCount(1);
    for (int row = 0; row < RowCount; ++row) {
        model.setData(model.index(row, 0), QColor(Qt::blue), Qt::BackgroundRole);
    }
    QCoreApplication::processEvents();
    const QImage image = themed.viewport_image();
    const int row_height = themed.table().rowHeight(0);

    const QImage first_row = image.copy(0, 0, image.width(), row_height);
    const QImage second_row = image.copy(0, row_height, image.width(), row_height);
    EXPECT_NE(first_row, second_row);
}

// A table draws the lines of its grid across what it shows: a vertical line does not change
// when the table scrolls along it.
TEST(ScrolledViewTest, GridLineAlongTheScrollStaysTheSame) {
    ThemedTable themed;
    // Empty cells, taller than the viewport: what it shows is the vertical lines of the grid.
    themed.table().setModel(new QStandardItemModel(RowCount, ColumnCount, &themed.table()));
    themed.table().setAlternatingRowColors(false);
    themed.table().verticalHeader()->setDefaultSectionSize(1000);
    themed.table().setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QCoreApplication::processEvents();
    const QImage before = themed.viewport_image();

    themed.scroll_by(QPoint(0, ScrollStep));
    const QImage after = themed.viewport_image();

    EXPECT_EQ(before, after);
}
