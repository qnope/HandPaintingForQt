#pragma once

#include <cstdint>

namespace hpq {

// `value` brought between 0 and `period`, `period` excluded, by whole periods.
int wrapped(int value, int period);

// A number in [0, 1) that depends only on the lattice point and the salt.
double lattice_value(int x, int y, std::uint32_t salt);

// Value noise in [0, 1): random values on a lattice of `cell_x` by `cell_y` cells, blended
// smoothly between them. The lattice repeats after `period_x` by `period_y` cells.
double value_noise(double x, double y, double cell_x, double cell_y, int period_x, int period_y, std::uint32_t salt);

// Square value noise of `cell` pixels that repeats every `size` pixels.
double square_noise(int x, int y, int cell, int size, std::uint32_t salt);

}  // namespace hpq
