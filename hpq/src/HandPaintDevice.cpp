#include <hpq/HandPaintDevice.h>

#include <QPainter>

// Defined and exported by QtGui (friend of QPaintDevice): reads a metric of any device.
Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric);

namespace hpq {

HandPaintDevice::HandPaintDevice(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine)
    : m_target(target), m_engine(target, theme, geometry_engine) {}

QPaintEngine *HandPaintDevice::paintEngine() const { return &m_engine; }

int HandPaintDevice::metric(PaintDeviceMetric metric) const {
    return qt_paint_device_metric(m_target.device(), metric);
}

}  // namespace hpq
