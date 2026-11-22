#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/GeometryEngine.h>

namespace hpq {

// Draws each straight segment of the ideal shape as theme.passes() strokes, each with randomly offset ends
// and a slight random bow, so that every stroke stays within theme.roughness() of the ideal segment.
// The fill area is the ideal shape.
class RoughGeometryEngine : public GeometryEngine
{
public:
    HandDrawnShape hand_draw(const QPainterPath& ideal, const Theme& theme, RandomGenerator& random) const override;
};

} // namespace hpq
