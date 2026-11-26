#pragma once

#include <hpq/Theme.h>

namespace hpq::test {

// `theme` drawn by a hand that does not wobble, in one stroke: the outline and the fill of a
// shape are the shape itself, so a test knows the paths that the target receives.
inline Theme exact(Theme theme) {
    theme.rough_options.roughness = 0.0;
    theme.rough_options.stroke_count = 1;
    return theme;
}

}  // namespace hpq::test
