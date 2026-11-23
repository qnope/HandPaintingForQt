#pragma once

namespace hpq {

// What a theme draws its strokes and fills with.
enum class Medium {
    // A clean line and a plain fill.
    Pencil,
    // A grainy line with uneven pressure, and fills rubbed on the grain of a slate.
    Chalk,
    // The line of a brush loaded with ink, which swells and thins with the pressure of the
    // hand and ends in a point, and a plain fill.
    Ink,
};

}  // namespace hpq
