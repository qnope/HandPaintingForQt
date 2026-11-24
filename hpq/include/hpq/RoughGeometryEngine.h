#pragma once

#include <hpq/Export.h>
#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>

#include <cstdint>

namespace hpq {

// Geometry engine that gives shapes a hand-drawn look: each stroke wobbles
// around the original shape, never farther than RoughOptions::max_offset.
// The wobble comes from the base seed of the engine and the seed of the shape:
// an engine draws a shape the same way every time, so a repaint changes nothing.
class HPQ_EXPORT RoughGeometryEngine : public GeometryEngine {
public:
    // Takes its base seed from std::random_device: each engine has its own hand.
    RoughGeometryEngine();
    // Engines with the same base seed draw the same paths.
    explicit RoughGeometryEngine(std::uint32_t base_seed);

    std::vector<QPainterPath> outline(const QPainterPath &shape, const RoughOptions &options,
                                      std::uint32_t seed) const override;
    std::vector<QPainterPath> frame_outline(const QPainterPath &frame, const RoughOptions &options,
                                            std::uint32_t seed) const override;
    QPainterPath fill_area(const QPainterPath &shape, const RoughOptions &options,
                           std::uint32_t seed) const override;
    // Straight lines, RoughOptions::hachure_gap apart, whose ends move a little. They lie on
    // a grid of the device, so that the hachures of two adjacent shapes line up.
    QPainterPath hachure(const QPainterPath &shape, const RoughOptions &options,
                         std::uint32_t seed) const override;

private:
    std::uint32_t m_base_seed;
};

}  // namespace hpq
