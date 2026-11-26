#pragma once

#include <QPainter>

namespace hpq {

// Saves the state of a painter, and restores it at the end of the scope: whatever is set on the
// painter in between, and however the scope is left, the painter is as it was.
class SavedPainter {
public:
    explicit SavedPainter(QPainter &painter) : m_painter(painter) { m_painter.save(); }
    ~SavedPainter() { m_painter.restore(); }

    SavedPainter(const SavedPainter &) = delete;
    SavedPainter &operator=(const SavedPainter &) = delete;

private:
    QPainter &m_painter;
};

}  // namespace hpq
