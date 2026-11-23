#pragma once

#include <QColor>
#include <QImage>

namespace hpq {

// The colors of a sheet of Seyes ruled paper.
struct SeyesColors {
    QColor paper;
    // The lines of the squares.
    QColor line;
    // The three lines between two lines of the squares.
    QColor fine_line;
    QColor margin;
};

// Side, in pixels, of a square of the ruling: 8 mm on the paper.
constexpr int SeyesSquare = 32;
// Distance, in pixels, between two horizontal lines: 2 mm on the paper.
constexpr int SeyesInterline = SeyesSquare / 4;
// Distance, in pixels, from the left of the sheet to its margin line.
constexpr int SeyesMargin = 2 * SeyesSquare;
// Width, in pixels, of the sheet. A brush repeats it: a second margin line shows on anything
// wider.
constexpr int SeyesSheetWidth = 128 * SeyesSquare;

// A sheet of Seyes ruled paper, the ruling of French school notebooks, one square high: a
// horizontal line every interline, heavier on the squares, a vertical line on every square at
// the right of the margin, and the margin line. It tiles: the sheets one under the other are
// the page.
QImage seyes_sheet(const SeyesColors &colors);

}  // namespace hpq
