#pragma once

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

// Watercolor is laid on `painter` in its logical coordinates, at the resolution of its device,
// over what the painter already drew: like a glaze, it darkens what is under it and never
// hides it, when the device can do that. The same paint laid twice at the same place is not
// darker than laid once.
// A translucent pigment lets as much of what is under it show unchanged.
// What is laid depends only on the arguments and on the place: a painter that draws a part of
// it through a clip draws the same pixels as one that draws it whole.
// Both functions leave the painter with another composition mode.

// Lays the wash that a brush loaded with `pigment` leaves on `area`. Its edge is soft and
// follows the grain of the paper; the pigment gathers there in a darker rim, and settles
// unevenly inside.
void paint_wash(QPainter &painter, const QPainterPath &area, const QColor &pigment, WashKind kind);

// Fills `ribbon`, the area of a line drawn with the tip of a brush loaded with `pigment`,
// with a smooth edge. The pigment settles unevenly along the line: darker here, paler there.
void paint_line(QPainter &painter, const QPainterPath &ribbon, const QColor &pigment);

}  // namespace hpq
