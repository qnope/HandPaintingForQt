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
};

}  // namespace hpq
