#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>

#include <QPaintDevice>

#include <memory>

class QPainter;

namespace hpq {

// Paint device that gives a hand-painted look to what is drawn on it: each primitive is drawn
// on the target painter, made rough by the geometry engine and drawn as the theme says.
// The target painter, the theme, the geometry engine and the cache must outlive the device.
// With a cache, what is made to draw a shape is kept there and taken back when the same shape
// is drawn again, by this device or by a later one with the same theme and geometry engine.
class HPQ_EXPORT HandPaintDevice : public QPaintDevice {
public:
    HandPaintDevice(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                    PaintCache *cache = nullptr);
    ~HandPaintDevice() override;

    QPaintEngine *paintEngine() const override;

protected:
    // The metrics of the target painter's device, so that sizes and DPI are the same, except
    // that the size is in logical pixels and the device pixel ratio is 1: the target painter
    // already scales to device pixels.
    int metric(PaintDeviceMetric metric) const override;

private:
    QPainter &m_target;
    // The paint engine is private to the library (src/HandPaintEngine.h).
    std::unique_ptr<HandPaintEngine> m_engine;
};

}  // namespace hpq
