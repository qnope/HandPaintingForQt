#include "SeyesPaper.h"

#include <QPainter>

namespace hpq {

QImage seyes_sheet(const SeyesColors &colors) {
    QImage sheet(SeyesSheetWidth, SeyesSquare, QImage::Format_RGB32);
    sheet.fill(colors.paper);
    QPainter painter(&sheet);
    for (int y = SeyesInterline; y < SeyesSquare; y += SeyesInterline) {
        painter.fillRect(0, y, SeyesSheetWidth, 1, colors.fine_line);
    }
    painter.fillRect(0, 0, SeyesSheetWidth, 1, colors.line);
    for (int x = SeyesMargin + SeyesSquare; x < SeyesSheetWidth; x += SeyesSquare) {
        painter.fillRect(x, 0, 1, SeyesSquare, colors.line);
    }
    painter.fillRect(SeyesMargin, 0, 1, SeyesSquare, colors.margin);
    painter.end();
    return sheet;
}

}  // namespace hpq
