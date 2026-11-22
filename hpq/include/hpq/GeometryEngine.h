#pragma once

#include <hpq/FwdDecls.h>

#include <QPainterPath>

#include <random>

namespace hpq {

// The random generator a geometry engine draws its randomness from; the device owns and seeds it.
using RandomGenerator = std::mt19937;

// What a geometry engine makes of an ideal shape.
struct HandDrawnShape
{
    // Stroked with the theme ink color and stroke width.
    QPainterPath strokes;
    // Filled with the theme fill color (only for a shape drawn with a brush).
    QPainterPath fill_area;
};

// Turns an ideal shape into the strokes to draw and the area to fill.
class GeometryEngine
{
public:
    virtual ~GeometryEngine() = default;

    // ideal: the shape in device coordinates, with its fill rule. A closed subpath ends with closeSubpath(),
    // an open one (a polyline, a line) does not.
    virtual HandDrawnShape hand_draw(const QPainterPath& ideal, const Theme& theme, RandomGenerator& random) const = 0;
};

} // namespace hpq
