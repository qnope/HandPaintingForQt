#include <hpq/HandPaintDevice.h>

#include <utility>

namespace hpq {

namespace {

// QPaintDevice::metric is protected: this makes it reachable on any paint device.
struct MetricAccess : QPaintDevice
{
    using QPaintDevice::metric;
};

int metric_of(const QPaintDevice& device, QPaintDevice::PaintDeviceMetric metric)
{
    return (device.*(&MetricAccess::metric))(metric);
}

// The default geometry engine: it gives back the ideal shape unchanged.
class PlainGeometryEngine : public GeometryEngine
{
public:
    HandDrawnShape hand_draw(const QPainterPath& ideal, const Theme& /*theme*/,
                             RandomGenerator& /*random*/) const override
    {
        return HandDrawnShape{ideal, ideal};
    }
};

} // namespace

HandPaintDevice::HandPaintDevice(QPaintDevice& target, Theme theme)
    : HandPaintDevice(target, std::move(theme), std::make_unique<PlainGeometryEngine>())
{
}

HandPaintDevice::HandPaintDevice(QPaintDevice& target, Theme theme, std::unique_ptr<GeometryEngine> geometry_engine)
    : m_target(target)
    , m_theme(std::move(theme))
    , m_geometry_engine(std::move(geometry_engine))
    , m_engine(*this)
{
}

QPaintEngine* HandPaintDevice::paintEngine() const { return &m_engine; }

int HandPaintDevice::metric(PaintDeviceMetric metric) const { return metric_of(m_target, metric); }

} // namespace hpq
