#pragma once

#include <QPainter>
#include <QPen>
#include <QWidget>

namespace hpq_tests {

// A 100x80 widget that draws the outline of the rect (10,10,80,60) with a pen and no brush.
class RectWidget : public QWidget
{
public:
    RectWidget() { resize(100, 80); }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);
        painter.setPen(QPen(Qt::black));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(10, 10, 80, 60);
    }
};

} // namespace hpq_tests
