#pragma once

namespace hpq {

// How a theme fills the inside of a shape.
enum class FillStyle {
    // The whole inside, in the brush.
    Solid,
    // Parallel lines in the brush, on the paper (the window color of the theme).
    Hachure,
};

}  // namespace hpq
