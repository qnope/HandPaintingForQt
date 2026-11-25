#pragma once

#include <hpq/FwdDecls.h>

#include <QColor>
#include <QPainterPath>

class QPainter;

namespace hpq {

// How a paint brush lays a wash on an area.
enum class WashKind {
    // A puddle of any shape: the pigment gathers all along its edge.
    Patch,
    // One stroke of a flat brush along the longer side of a rectangle: the pigment gathers
    // along its two long edges only, and the stroke overruns its two ends a little. The bands
    // of the cells of a row join without a seam.
    Band,
};

// Lays on `painter` the wash that a brush loaded with `pigment` leaves on `area`, in the
// logical coordinates of the painter and at the resolution of its device. Its edge is soft
// and follows the grain of the paper; the pigment gathers there in a darker rim, and settles
// unevenly inside.
// The wash is laid over what the painter already drew: like a glaze, it darkens what is under
// it and never hides it, when the device can do that. The same wash laid twice at the same
// place is not darker than laid once. A translucent pigment lets as much of what is under it
// show unchanged.
// What is laid depends only on the arguments and on the place: a painter that draws a part of
// the wash through a clip draws the same pixels as one that draws it whole.
// Leaves the painter with another composition mode.
// With a cache, the wash is made whole the first time and kept there, then taken back when
// the same wash is laid again, at any place: what is laid is the same.
void paint_wash(QPainter &painter, const QPainterPath &area, const QColor &pigment, WashKind kind,
                PaintCache *cache = nullptr);

// How far past its area, in logical pixels of the painter, a wash can change a pixel.
double wash_reach();

}  // namespace hpq
