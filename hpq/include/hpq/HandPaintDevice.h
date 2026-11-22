#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintEngine.h>
#include <hpq/Theme.h>

#include <QPaintDevice>

#include <memory>

namespace hpq {

// A paint device that draws, hand-painted with a theme, on a target paint device.
// The target must outlive the device; its size and resolution are the device ones.
class HandPaintDevice : public QPaintDevice
{
public:
    // The seed of a device whose seed is not set.
    static constexpr Seed DefaultSeed = RandomGenerator::default_seed;

    // Uses a RoughGeometryEngine.
    HandPaintDevice(QPaintDevice& target, Theme theme);
    // geometry_engine must not be null.
    HandPaintDevice(QPaintDevice& target, Theme theme, std::unique_ptr<GeometryEngine> geometry_engine);

    QPaintDevice& target() const { return m_target; }
    const Theme& theme() const { return m_theme; }
    const GeometryEngine& geometry_engine() const { return *m_geometry_engine; }

    // The seed of the random strokes. The engine reseeds its generator with it at each begin(), so
    // painting the same drawing gives the same image, on this device or on another with the same seed.
    Seed seed() const { return m_seed; }
    void set_seed(Seed seed) { m_seed = seed; }

    QPaintEngine* paintEngine() const override;

protected:
    int metric(PaintDeviceMetric metric) const override;

private:
    QPaintDevice& m_target;
    Theme m_theme;
    std::unique_ptr<GeometryEngine> m_geometry_engine;
    Seed m_seed = DefaultSeed;
    mutable HandPaintEngine m_engine;
};

} // namespace hpq
