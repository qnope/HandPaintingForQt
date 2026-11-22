#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/HandPaintEngine.h>

#include <QPaintDevice>

class QPainter;

namespace hpq {

// Paint device that gives a hand-painted look to what is drawn on it: each primitive goes
// through a HandPaintEngine, which draws it on the target painter.
// The target painter, the theme and the geometry engine must outlive the device.
class HandPaintDevice : public QPaintDevice {
public:
    HandPaintDevice(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine);

    QPaintEngine *paintEngine() const override;

protected:
    // The metrics of the target painter's device, so that sizes and DPI are the same.
    int metric(PaintDeviceMetric metric) const override;

private:
    QPainter &m_target;
    mutable HandPaintEngine m_engine;
};

}  // namespace hpq
