#include <hpq/HandPaintDevice.h>

#include <QPainter>

// Defined and exported by QtGui (friend of QPaintDevice): reads a metric of any device.
Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric);

namespace hpq {

HandPaintDevice::HandPaintDevice(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine)
    : m_target(target), m_engine(target, theme, geometry_engine) {}

QPaintEngine *HandPaintDevice::paintEngine() const { return &m_engine; }

int HandPaintDevice::metric(PaintDeviceMetric metric) const {
    const QPaintDevice *target_device = m_target.device();
    switch (metric) {
    // The target painter already scales logical coordinates to device pixels: a device pixel
    // ratio above 1 here would make the painter of this device scale them a second time.
    // The ratio is given here rather than left to QPaintDevice::metric(), which warns that the
    // device has no metric information before Qt 6.8.
    case PdmDevicePixelRatio:
        return 1;
    case PdmDevicePixelRatioScaled:
        return static_cast<int>(devicePixelRatioFScale());
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)  // The encoded ratio metrics appeared in Qt 6.8.
    case PdmDevicePixelRatioF_EncodedA:
    case PdmDevicePixelRatioF_EncodedB:
        return QPaintDevice::metric(metric);
#endif
    // So the size is in the logical coordinates of the target painter.
    case PdmWidth:
        return qRound(qt_paint_device_metric(target_device, metric) / target_device->devicePixelRatio());
    case PdmHeight:
        return qRound(qt_paint_device_metric(target_device, metric) / target_device->devicePixelRatio());
    default:
        return qt_paint_device_metric(target_device, metric);
    }
}

}  // namespace hpq
