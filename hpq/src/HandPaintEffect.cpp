#include <hpq/HandPaintDevice.h>
#include <hpq/HandPaintEffect.h>

#include <QAbstractScrollArea>
#include <QPainter>
#include <QScrollArea>
#include <QPaintEngine>
#include <QRegion>
#include <QTransform>
#include <QWidget>

#include <utility>

namespace hpq {

HandPaintEffect::HandPaintEffect(Theme theme,
                                 std::shared_ptr<const GeometryEngine> geometry_engine,
                                 QObject *parent)
    : QGraphicsEffect(parent),
      m_theme(std::move(theme)),
      m_geometry_engine(std::move(geometry_engine)) {}

namespace {

// How far, in pixels, from a shape its hand-painted strokes can put ink: what is repainted
// must be drawn again with every shape this close to it.
constexpr int InkOverflow = 16;

// The area that Qt repaints with `painter`, in its coordinates; empty when it is not limited.
QRect repainted_area(const QPainter &painter) {
    const QRegion system_clip = painter.paintEngine()->systemClip();
    const QTransform to_device = painter.deviceTransform();
    if (system_clip.isEmpty() || !to_device.isInvertible()) {
        return QRect();
    }
    return to_device.inverted().mapRect(QRectF(system_clip.boundingRect())).toAlignedRect();
}

// True when `widget` may draw shapes that depend on the area it is asked to paint: a widget
// without children, or the viewport of a view (a table draws its grid lines across that area).
// Its rough shapes are the same at every repaint only if it always paints its whole area.
// A widget that only holds other widgets fills its background, which looks the same in parts.
bool paints_whole(const QWidget &widget) {
    const auto *scroll_area = qobject_cast<const QAbstractScrollArea *>(widget.parentWidget());
    if (scroll_area && scroll_area->viewport() == &widget && !qobject_cast<const QScrollArea *>(scroll_area)) {
        return true;
    }
    for (const QObject *child : widget.children()) {
        const auto *child_widget = qobject_cast<const QWidget *>(child);
        if (child_widget && !child_widget->isHidden() && !child_widget->isWindow()) {
            return false;
        }
    }
    return true;
}

// Adds to `region` (in the coordinates of `source`) the whole area of the widgets under
// `widget` that touch `area` and paint whole. `visible` is what the ancestors let show.
void add_whole_widgets(const QWidget &widget, const QWidget &source, const QRect &area, const QRect &visible,
                       QRegion &region) {
    for (QObject *child : widget.children()) {
        const auto *child_widget = qobject_cast<const QWidget *>(child);
        if (!child_widget || child_widget->isHidden() || child_widget->isWindow()) {
            continue;
        }
        const QRect child_area =
            QRect(child_widget->mapTo(&source, QPoint()), child_widget->size()).intersected(visible);
        if (!child_area.intersects(area)) {
            continue;
        }
        if (paints_whole(*child_widget)) {
            region += child_area;
        } else {
            add_whole_widgets(*child_widget, source, area, child_area, region);
        }
    }
}

}  // namespace

void HandPaintEffect::draw(QPainter *painter) {
    HandPaintDevice device(*painter, m_theme, *m_geometry_engine, &m_path_cache);
    QPainter hand_painter(&device);
    // The painter is not the one of the paint context, so Qt renders the source widget
    // (and its children) on it; the effect is not applied again inside this render.
    auto *widget = qobject_cast<QWidget *>(parent());
    const QRect repainted = repainted_area(*painter);
    if (!widget || widget->graphicsEffect() != this || repainted.isEmpty()) {
        drawSource(&hand_painter);
        return;
    }
    // drawSource() renders the whole widget, whatever is repainted: a hover on a button would
    // draw every shape of the window again. Render only what is near the repainted area.
    const QRect near_repainted =
        repainted.adjusted(-InkOverflow, -InkOverflow, InkOverflow, InkOverflow).intersected(widget->rect());
    if (near_repainted.isEmpty()) {
        return;
    }
    QRegion rendered(near_repainted);
    add_whole_widgets(*widget, *widget, near_repainted, widget->rect(), rendered);
    widget->render(&hand_painter, rendered.boundingRect().topLeft(), rendered);
}

}  // namespace hpq
