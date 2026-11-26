#include "ContentOrigin.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QPaintDevice>
#include <QPainter>
#include <QScrollBar>
#include <QWidget>

namespace hpq {

namespace {

// Reads the scroll offsets of a view of items, which QAbstractItemView keeps protected: a
// pointer to a member, taken through a derived class, calls them on any view. They are in
// pixels, also when the view scrolls by items.
class ItemViewOffsets : public QAbstractItemView {
public:
    static QPointF of(const QAbstractItemView &view) {
        return QPointF((view.*&ItemViewOffsets::horizontalOffset)(), (view.*&ItemViewOffsets::verticalOffset)());
    }
};

// The scroll area whose viewport is `widget`; null when it is not a viewport.
const QAbstractScrollArea *scroll_area_of(const QWidget &widget) {
    const auto *area = qobject_cast<const QAbstractScrollArea *>(widget.parentWidget());
    return area != nullptr && area->viewport() == &widget ? area : nullptr;
}

// How far the content shown by `device` is scrolled: none, unless it is the viewport of a
// scroll area.
QPointF scroll_offset(const QPaintDevice *device) {
    const QWidget *widget = widget_of(device);
    const QAbstractScrollArea *area = widget != nullptr ? scroll_area_of(*widget) : nullptr;
    if (area == nullptr) {
        return QPointF();
    }
    if (const auto *view = qobject_cast<const QAbstractItemView *>(area)) {
        return ItemViewOffsets::of(*view);
    }
    return QPointF(area->horizontalScrollBar()->value(), area->verticalScrollBar()->value());
}

}  // namespace

const QWidget *widget_of(const QPaintDevice *device) {
    if (device == nullptr || device->devType() != QInternal::Widget) {
        return nullptr;
    }
    return static_cast<const QWidget *>(device);
}

QRect drawable_rect(const QWidget &widget, int margin) {
    QRect drawable = widget.rect().adjusted(-margin, -margin, margin, margin);
    for (const QWidget *ancestor = &widget; !ancestor->isWindow(); ancestor = ancestor->parentWidget()) {
        if (scroll_area_of(*ancestor) != nullptr) {
            drawable &= QRect(widget.mapFrom(ancestor, QPoint()), ancestor->size());
        }
    }
    return drawable;
}

// `to_device` maps the logical coordinates of the device painter to the device. Its combined
// transform maps them to the widget being painted: the two differ by where that widget is on
// the device.
QPointF widget_origin(const QPainter &device_painter, const QTransform &to_device) {
    return to_device.map(QPointF()) - device_painter.combinedTransform().map(QPointF());
}

QPointF content_origin(const QPainter &device_painter, const QTransform &to_device) {
    return widget_origin(device_painter, to_device) - scroll_offset(device_painter.device());
}

QPointF line_origin(const QPainter &device_painter, const QTransform &to_device, const QPainterPath &logical_line) {
    const QPaintDevice *widget = device_painter.device();
    const QRectF line = device_painter.combinedTransform().map(logical_line).boundingRect();
    QPointF origin = content_origin(device_painter, to_device);
    // A pen of one pixel draws through the middle of the pixels: half a pixel from the edges.
    if (line.height() == 0.0 && line.left() <= 1.0 && line.right() >= widget->width() - 1.0) {
        origin.setX(widget_origin(device_painter, to_device).x());
    }
    if (line.width() == 0.0 && line.top() <= 1.0 && line.bottom() >= widget->height() - 1.0) {
        origin.setY(widget_origin(device_painter, to_device).y());
    }
    return origin;
}

}  // namespace hpq
