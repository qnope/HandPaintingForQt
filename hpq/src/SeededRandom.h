#pragma once

#include <cstdint>

namespace hpq {

// Small deterministic random generator (SplitMix64). Unlike the standard
// distributions, it gives the same numbers with every compiler and platform.
class SeededRandom {
public:
    explicit SeededRandom(std::uint64_t seed) : m_state(seed) {}

    // A number in [-1, 1].
    double next_signed()
    {
        const double unit = static_cast<double>(next() >> 11) / static_cast<double>(1ULL << 53);
        return unit * 2.0 - 1.0;
    }

private:
    std::uint64_t next()
    {
        m_state += 0x9E3779B97F4A7C15ULL;
        std::uint64_t value = m_state;
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    std::uint64_t m_state;
};

}  // namespace hpq
