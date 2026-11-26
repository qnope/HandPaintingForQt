#include "Target.h"

#include <hpq/PaintCache.h>

#include <QGradient>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <QRegion>
#include <QSizeF>
#include <QTransform>

#include <cmath>
#include <vector>

namespace hpq {

namespace {

// `brush` laid on a shape whose control points are in `bounds`, on a device of `device_size`,
// with its gradient in the logical coordinates of the painter, as Qt lays a gradient given in
// the coordinates of the shape or of the device: drawn in pieces, or with the painter moved, the
// shape keeps its gradient.
QBrush laid_on(const QBrush &brush, const QRectF &bounds, const QSizeF &device_size) {
    const QGradient *gradient = brush.gradient();
    if (gradient == nullptr || gradient->coordinateMode() == QGradient::LogicalMode) {
        return brush;
    }
    const QRectF stretch =
        gradient->coordinateMode() == QGradient::StretchToDeviceMode ? QRectF(QPointF(), device_size) : bounds;
    const QTransform to_logical(stretch.width(), 0.0, 0.0, stretch.height(), stretch.x(), stretch.y());
    QGradient logical = *gradient;
    logical.setCoordinateMode(QGradient::LogicalMode);
    QBrush laid(logical);
    // As QPainter stretches a gradient: in ObjectMode, the transform of the brush is in the
    // coordinates of the shape; in the other modes, in those of the gradient.
    laid.setTransform(gradient->coordinateMode() == QGradient::ObjectMode ? brush.transform() * to_logical
                                                                          : to_logical * brush.transform());
    return laid;
}

// The pieces of `area` for a target that maps it to the device with `to_device`: those
// that reach the `paintable` pixels, and maybe others. With a cache, they are kept there.
std::vector<QPainterPath> pieces_of(const QPainterPath &area, const QTransform &to_device, const QRect &paintable,
                                    Tiling tiling, PaintCache *cache) {
    if (fits_in_a_piece(to_device.mapRect(area.boundingRect()).size(), tiling)) {
        return {area};
    }
    // A target that turns or skews the content gets pieces for this drawing only.
    if (to_device.type() > QTransform::TxScale) {
        return path_tiles(area, to_device, paintable, tiling);
    }
    // The pieces end between two pixels of the device. Cut from the fraction of a pixel where
    // the content starts, they are the same wherever the content is moved by whole pixels, as
    // when a view scrolls: they are kept for the scale and that fraction.
    const QPointF start = to_device.map(QPointF());
    const QPoint whole(static_cast<int>(std::floor(start.x())), static_cast<int>(std::floor(start.y())));
    const QPointF phase = start - QPointF(whole);
    const QTransform cut(to_device.m11(), 0.0, 0.0, to_device.m22(), phase.x(), phase.y());
    if (cache == nullptr || to_device.m11() != to_device.m22()) {
        return path_tiles(area, cut, paintable.translated(-whole), tiling);
    }
    return cache->pieces(area, static_cast<int>(tiling), to_device.m11(), phase, [&] {
        return path_tiles(area, cut, cut.mapRect(area.boundingRect()).toAlignedRect(), tiling);
    });
}

}  // namespace

QRect paintable_pixels(const QPainter &target, const QRectF &area) {
    const QTransform to_device = target.deviceTransform();
    QRect pixels = to_device.mapRect(area).toAlignedRect();
    if (target.hasClipping()) {
        pixels &= to_device.mapRect(target.clipBoundingRect()).toAlignedRect();
    }
    const QRegion system_clip = target.paintEngine()->systemClip();
    if (!system_clip.isEmpty()) {
        pixels &= system_clip.boundingRect();
    }
    return pixels;
}

QBrush moved(QBrush brush, const QPointF &offset) {
    brush.setTransform(brush.transform() * QTransform::fromTranslate(offset.x(), offset.y()));
    return brush;
}

void set_to_content(QPainter &target, const QPainterPath &path, const QPen &pen, const QBrush &brush,
                    const QPointF &origin) {
    const QRectF bounds = path.controlPointRect().translated(origin);
    const QSizeF device_size(target.device()->width(), target.device()->height());
    // The brushes are laid where they were before the target moved.
    QPen moved_pen = pen;
    moved_pen.setBrush(moved(laid_on(pen.brush(), bounds, device_size), -origin));
    target.translate(origin);
    target.setPen(moved_pen);
    target.setBrush(moved(laid_on(brush, bounds, device_size), -origin));
}

void fill_in_pieces(QPainter &target, const QPainterPath &area, const QBrush &brush, Tiling tiling,
                    const QPointF &origin, PaintCache *cache) {
    set_to_content(target, area, Qt::NoPen, brush, origin);
    // Another engine might smooth each piece on its own, and show the cuts.
    if (target.paintEngine()->type() != QPaintEngine::Raster) {
        target.drawPath(area);
        return;
    }
    const QRect paintable = paintable_pixels(target, area.boundingRect());
    if (paintable.isEmpty()) {
        return;
    }
    const QTransform to_device = target.deviceTransform();
    for (const QPainterPath &piece : pieces_of(area, to_device, paintable, tiling, cache)) {
        if (to_device.mapRect(piece.boundingRect()).toAlignedRect().intersects(paintable)) {
            target.drawPath(piece);
        }
    }
}

}  // namespace hpq
