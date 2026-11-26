#pragma once

#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

namespace hpq::test {

// `theme` with `options` in place of its rough options.
Theme with_rough_options(const Theme &theme, const RoughOptions &options);

// `theme` drawn by a hand that does not wobble, in one stroke: the outline and the fill of a
// shape are the shape itself, so a test knows the paths that the target receives.
Theme exact(const Theme &theme);

}  // namespace hpq::test
