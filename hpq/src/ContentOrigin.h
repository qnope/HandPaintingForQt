#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QTransform>

class QPaintDevice;
class QPainter;
class QWidget;

namespace hpq {

// Where the content of the widget being painted is on the device. A hand paint engine makes a
// shape rough in the coordinates of that content, so that the shape keeps its drawing when its
// view scrolls.
// Below, `device_painter` is the painter of the hand paint device, and `to_device` the transform
// of its state, which maps its logical coordinates to the device.

// The widget that `device` is; null when it is another device.
const QWidget *widget_of(const QPaintDevice *device);

// Where `widget` may draw, in its coordinates: up to `margin` past its rectangle, but not past
// the viewport of a scroll area that shows it. A viewport cuts what it shows at its edges:
// past them is the frame of its scroll area, and what is around it.
QRect drawable_rect(const QWidget &widget, int margin);

// Where the origin of the widget being painted is on the device; the origin of the device
// when the painter does not paint a widget.
QPointF widget_origin(const QPainter &device_painter, const QTransform &to_device);

// Where the origin of the content of the widget being painted is on the device: the origin
// of the widget, moved back by how far it scrolled its content when it is the viewport of a
// scroll area.
QPointF content_origin(const QPainter &device_painter, const QTransform &to_device);

// The origin of `logical_line`: the one of the content, but along a horizontal or vertical
// line that crosses the whole widget. A view draws the lines of its grid across what it
// shows, not across its whole content: placed along its length in the widget, such a line
// does not change when the view scrolls.
QPointF line_origin(const QPainter &device_painter, const QTransform &to_device, const QPainterPath &logical_line);

}  // namespace hpq
