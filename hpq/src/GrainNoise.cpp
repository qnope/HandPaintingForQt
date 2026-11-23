#include "GrainNoise.h"

#include <cmath>

namespace hpq {

namespace {

double smooth(double t) {
    return t * t * (3.0 - 2.0 * t);
}

}  // namespace

int wrapped(int value, int period) {
    const int remainder = value % period;
    return remainder < 0 ? remainder + period : remainder;
}

double lattice_value(int x, int y, std::uint32_t salt) {
    std::uint32_t hash = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u ^
                         salt * 83492791u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    hash ^= hash >> 16;
    return hash / 4294967296.0;
}

double value_noise(double x, double y, double cell_x, double cell_y, int period_x, int period_y, std::uint32_t salt) {
    const double lattice_x = x / cell_x;
    const double lattice_y = y / cell_y;
    const int left = static_cast<int>(std::floor(lattice_x));
    const int top = static_cast<int>(std::floor(lattice_y));
    const double tx = smooth(lattice_x - left);
    const double ty = smooth(lattice_y - top);
    const auto corner = [&](int dx, int dy) {
        return lattice_value(wrapped(left + dx, period_x), wrapped(top + dy, period_y), salt);
    };
    const double upper = (1.0 - tx) * corner(0, 0) + tx * corner(1, 0);
    const double lower = (1.0 - tx) * corner(0, 1) + tx * corner(1, 1);
    return (1.0 - ty) * upper + ty * lower;
}

double square_noise(int x, int y, int cell, int size, std::uint32_t salt) {
    return value_noise(x, y, cell, cell, size / cell, size / cell, salt);
}

}  // namespace hpq
