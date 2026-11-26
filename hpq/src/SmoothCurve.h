#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace hpq {

// A value that varies smoothly along a path of a given length: values at evenly spaced knots,
// blended with a cosine between two knots.
template <typename Value>
class SmoothCurve {
public:
    // The knots are about `wanted_spacing` apart; `make_knot` gives the value of each knot, in
    // the order of the path. A closed path ends where it starts: with the same value there.
    template <typename MakeKnot>
    SmoothCurve(double path_length, double wanted_spacing, bool closed, MakeKnot make_knot) {
        const int interval_count = std::max(1, static_cast<int>(std::lround(path_length / wanted_spacing)));
        m_knot_spacing = path_length / interval_count;
        for (int index = 0; index <= interval_count; ++index) {
            m_knots.push_back(make_knot());
        }
        if (closed) {
            m_knots.back() = m_knots.front();
        }
    }

    Value at(double arc_length) const {
        if (m_knot_spacing <= 0.0) {
            return m_knots.front();
        }
        constexpr double Pi = 3.14159265358979323846;
        const double position = arc_length / m_knot_spacing;
        const int last_interval = static_cast<int>(m_knots.size()) - 2;
        const int interval = std::clamp(static_cast<int>(std::floor(position)), 0, last_interval);
        const double t = std::clamp(position - interval, 0.0, 1.0);
        const double weight = (1.0 - std::cos(Pi * t)) / 2.0;
        return (1.0 - weight) * m_knots[interval] + weight * m_knots[interval + 1];
    }

private:
    std::vector<Value> m_knots;
    double m_knot_spacing = 0.0;
};

// 0 at `from` and below, 1 at `to` and above, and a smooth step between them.
inline double smooth_step(double from, double to, double value) {
    const double t = std::clamp((value - from) / (to - from), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

}  // namespace hpq
