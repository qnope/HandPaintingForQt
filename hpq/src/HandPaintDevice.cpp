#include <hpq/HandPaintDevice.h>

#include "HandPaintEngine.h"

#include <QPainter>

namespace hpq {

HandPaintDevice::HandPaintDevice(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                                 PaintCache *cache)
    : m_target(target), m_engine(std::make_unique<HandPaintEngine>(target, theme, geometry_engine, cache)) {}

HandPaintDevice::~HandPaintDevice() = default;

QPaintEngine *HandPaintDevice::paintEngine() const { return m_engine.get(); }

int HandPaintDevice::metric(PaintDeviceMetric metric) const {
    const QPaintDevice &target_device = *m_target.device();
    switch (metric) {
    // The target painter already scales logical coordinates to device pixels: a device pixel
    // ratio above 1 here would make the painter of this device scale them a second time.
    // The ratio is given here rather than left to QPaintDevice::metric(), which warns that the
    // device has no metric information before Qt 6.8.
    case PdmDevicePixelRatio:
        return 1;
    case PdmDevicePixelRatioScaled:
        return static_cast<int>(devicePixelRatioFScale());
    // So the size is in the logical coordinates of the target painter.
    case PdmWidth:
        return qRound(target_device.width() / target_device.devicePixelRatio());
    case PdmHeight:
        return qRound(target_device.height() / target_device.devicePixelRatio());
    case PdmWidthMM:
        return target_device.widthMM();
    case PdmHeightMM:
        return target_device.heightMM();
    case PdmNumColors:
        return target_device.colorCount();
    case PdmDepth:
        return target_device.depth();
    case PdmDpiX:
        return target_device.logicalDpiX();
    case PdmDpiY:
        return target_device.logicalDpiY();
    case PdmPhysicalDpiX:
        return target_device.physicalDpiX();
    case PdmPhysicalDpiY:
        return target_device.physicalDpiY();
    // The encoded ratio metrics of Qt 6.8, which follow from the ratio above.
    default:
        return QPaintDevice::metric(metric);
    }
}

}  // namespace hpq
