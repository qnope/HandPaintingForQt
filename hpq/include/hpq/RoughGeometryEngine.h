#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/GeometryEngine.h>

namespace hpq {

// Draws each straight segment of the ideal shape as theme.passes() strokes, each with randomly offset ends
// and a slight random bow, so that every stroke stays within theme.roughness() of the ideal segment.
// Each run of consecutive curves is flattened, then drawn as theme.passes() smooth curves whose points move
// along the normal by a smooth random offset of at most theme.roughness() / 2.
// The fill area is the ideal shape.
class RoughGeometryEngine : public GeometryEngine
{
public:
    HandDrawnShape hand_draw(const QPainterPath& ideal, const Theme& theme, RandomGenerator& random) const override;
};

} // namespace hpq
