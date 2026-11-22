#pragma once

namespace hpq {

// How much a geometry engine deforms a shape.
struct RoughOptions {
    // Scale of the deformation: 0 keeps the exact geometry, 1 uses the full max_offset.
    double roughness = 1.0;
    // Number of outline strokes drawn for one shape.
    int stroke_count = 2;
    // Largest distance, in pixels, between a rough point and the original shape.
    double max_offset = 2.0;
    // Distance, in pixels, between two hachures of a hatched fill.
    double hachure_gap = 4.0;
    // Angle of the hachures, in degrees: 0 is horizontal, a negative angle rises to the right.
    double hachure_angle = -41.0;
};

}  // namespace hpq
