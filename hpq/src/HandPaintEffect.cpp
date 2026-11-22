#include <hpq/HandPaintEffect.h>

#include <hpq/HandPaintDevice.h>

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRegion>

#include <utility>

namespace hpq {

namespace {

QWidget* widget_with_effect(const QGraphicsEffect& effect)
{
    for (QWidget* widget : QApplication::allWidgets()) {
        if (widget->graphicsEffect() == &effect) {
            return widget;
        }
    }
    return nullptr;
}

} // namespace

HandPaintEffect::HandPaintEffect(Theme theme, QObject* parent)
    : QGraphicsEffect(parent)
    , m_theme(std::move(theme))
{
}

void HandPaintEffect::set_theme(Theme theme)
{
    m_theme = std::move(theme);
    update();
}

void HandPaintEffect::sourceChanged(ChangeFlags flags)
{
    // Moving the effect to another widget detaches and attaches it in one call.
    if (flags.testFlag(SourceDetached)) {
        m_source_widget = nullptr;
    }
    if (flags.testFlag(SourceAttached)) {
        m_source_widget = widget_with_effect(*this);
    }
}

void HandPaintEffect::draw(QPainter* painter)
{
    const QRect source_rect = sourceBoundingRect(Qt::LogicalCoordinates).toAlignedRect();
    const qreal pixel_ratio = painter->device()->devicePixelRatioF();
    // The image has the physical size and a pixel ratio of 1 while painted, so that the scale to
    // physical pixels is applied once, by the device painter.
    QImage image(source_rect.size() * pixel_ratio, QImage::Format_ARGB32_Premultiplied);
    {
        HandPaintDevice device(image, m_theme);
        QPainter device_painter(&device);
        device_painter.scale(pixel_ratio, pixel_ratio);
        device_painter.translate(-source_rect.topLeft());
        draw_source_without_background(device_painter);
    }
    image.setDevicePixelRatio(pixel_ratio);
    painter->drawImage(source_rect.topLeft(), image);
}

void HandPaintEffect::draw_source_without_background(QPainter& painter)
{
    // drawSource() renders a widget with its window background, which the device would paint as a fill;
    // the device already paints the theme background. While the effect draws, the widget renders
    // without its effect, so this does not recurse.
    // The region is explicit: QWidget::render(QPainter*) (also used by render(QPaintDevice*)) marks the
    // widget as rendering with a painter, and a nested render of it then paints only the given region,
    // so the default empty region would paint nothing.
    if (m_source_widget) {
        m_source_widget->render(&painter, QPoint(), QRegion(m_source_widget->rect()), QWidget::DrawChildren);
    } else {
        drawSource(&painter);
    }
}

} // namespace hpq
