#pragma once

#include "PathTiles.h"

#include <hpq/FwdDecls.h>

#include <QBrush>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRect>
#include <QRectF>

class QPainter;

namespace hpq {

// What a hand paint engine does on its target painter alone, once the painter is clipped.
// A path is in the coordinates of the content of the widget being painted: in device pixels,
// counted from an `origin` on the device.

// The pixels of the device of `target` that it can paint in `area`: those in its clip, and in
// the system clip of its engine.
QRect paintable_pixels(const QPainter &target, const QRectF &area);

// `brush` with its image moved by `offset`: the image follows what is drawn there.
QBrush moved(QBrush brush, const QPointF &offset);

// Moves `target` to `origin`, and gives it `pen` and `brush`, that are laid on `path`: the
// target then draws `path`, in the coordinates of the content, as it would draw `path` moved
// to `origin`.
void set_to_content(QPainter &target, const QPainterPath &path, const QPen &pen, const QBrush &brush,
                    const QPointF &origin);

// Fills `area` with `brush` and the state of `target`, which it moves to `origin`: on the
// raster engine, which smooths a large path slowly, in the pieces of `tiling`. With a cache,
// the pieces are kept there; `cache` is null when they are cut at each drawing.
void fill_in_pieces(QPainter &target, const QPainterPath &area, const QBrush &brush, Tiling tiling,
                    const QPointF &origin, PaintCache *cache);

}  // namespace hpq
