#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/HandPaintEngine.h>
#include <hpq/Theme.h>

#include <QPaintDevice>

namespace hpq {

// A paint device that draws, hand-painted with a theme, on a target paint device.
// The target must outlive the device; its size and resolution are the device ones.
class HandPaintDevice : public QPaintDevice
{
public:
    HandPaintDevice(QPaintDevice& target, Theme theme);

    QPaintDevice& target() const { return m_target; }
    const Theme& theme() const { return m_theme; }

    QPaintEngine* paintEngine() const override;

protected:
    int metric(PaintDeviceMetric metric) const override;

private:
    QPaintDevice& m_target;
    Theme m_theme;
    mutable HandPaintEngine m_engine;
};

} // namespace hpq
