#include "StyleRendering.h"

#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>

#include <algorithm>
#include <string>
#include <vector>

namespace {

using hpq::test::DrawCall;

constexpr int CellCount = 3;
constexpr int TableSize = 200;
// Small enough for the 3x3 cells to fit in the table without scroll bars.
constexpr int ColumnWidth = 50;
constexpr int RowHeight = 30;
// Shapes may sit up to one pixel inside or outside the section rectangle (pen width).
constexpr int Tolerance = 1;

// The section rectangles of the header, in the coordinates of the table.
std::vector<QRect> section_rects(const QTableView &table, const QHeaderView &header) {
    std::vector<QRect> rects;
    const QWidget &viewport = *header.viewport();
    for (int section = 0; section < header.count(); ++section) {
        const int start = header.sectionViewportPosition(section);
        const int size = header.sectionSize(section);
        const QRect in_viewport = header.orientation() == Qt::Horizontal
                                      ? QRect(start, 0, size, viewport.height())
                                      : QRect(0, start, viewport.width(), size);
        rects.push_back(QRect(viewport.mapTo(&table, in_viewport.topLeft()), in_viewport.size()));
    }
    return rects;
}

// True when the shape of the call, on the device, lies at the rectangle.
bool is_at(const DrawCall &call, const QRect &rect) {
    const QRectF shape = call.transform.map(call.path).boundingRect();
    const QRectF outer = QRectF(rect).adjusted(-Tolerance, -Tolerance, Tolerance, Tolerance);
    return outer.contains(shape) && shape.width() >= rect.width() - 2 * Tolerance &&
           shape.height() >= rect.height() - 2 * Tolerance;
}

std::string describe(const QRect &rect) {
    return std::to_string(rect.x()) + "," + std::to_string(rect.y()) + " " + std::to_string(rect.width()) + "x" +
           std::to_string(rect.height());
}

bool is_filled_with(const DrawCall &call, const QColor &color) {
    return call.kind == DrawCall::Kind::Path && call.brush.style() == Qt::SolidPattern &&
           call.brush.color() == color;
}

bool is_outlined_with(const DrawCall &call, const QColor &color) {
    return call.kind == DrawCall::Kind::Path && call.pen.style() != Qt::NoPen && call.pen.color() == color;
}

bool has_call(const std::vector<DrawCall> &calls, const QRect &rect,
              bool (*predicate)(const DrawCall &, const QColor &), const QColor &color) {
    return std::any_of(calls.begin(), calls.end(),
                       [&](const DrawCall &call) { return predicate(call, color) && is_at(call, rect); });
}

class HeaderSectionTest : public ::testing::Test {
protected:
    HeaderSectionTest() : m_model(CellCount, CellCount) {
        m_table.setModel(&m_model);
        m_table.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table.resize(TableSize, TableSize);
        m_table.horizontalHeader()->setDefaultSectionSize(ColumnWidth);
        m_table.verticalHeader()->setDefaultSectionSize(RowHeight);
    }

    void expect_flat_sections(const std::vector<DrawCall> &calls, const QHeaderView &header) const {
        const QPalette palette = hpq::SketchTheme().palette();
        const std::vector<QRect> rects = section_rects(m_table, header);
        ASSERT_EQ(rects.size(), static_cast<size_t>(CellCount));
        for (const QRect &rect : rects) {
            EXPECT_TRUE(has_call(calls, rect, is_filled_with, palette.color(QPalette::Button)))
                << "no Button fill at section " << describe(rect);
            EXPECT_TRUE(has_call(calls, rect, is_outlined_with, palette.color(QPalette::Dark)))
                << "no Dark outline at section " << describe(rect);
        }
    }

    QStandardItemModel m_model;
    QTableView m_table;
};

TEST_F(HeaderSectionTest, TableHeadersAreFlatSectionsWithoutImage) {
    const std::vector<DrawCall> calls = hpq::test::render_with_style(m_table);

    EXPECT_FALSE(hpq::test::has_image(calls));
    expect_flat_sections(calls, *m_table.horizontalHeader());
    expect_flat_sections(calls, *m_table.verticalHeader());
}

}  // namespace
