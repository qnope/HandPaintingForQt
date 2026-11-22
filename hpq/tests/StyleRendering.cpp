#include "StyleRendering.h"

#include <hpq/HandPaintStyle.h>
#include <hpq/SketchTheme.h>

#include <QList>
#include <QWidget>

#include <algorithm>

namespace hpq::test {

namespace {

QList<QWidget *> widget_and_descendants(QWidget &widget) {
    QList<QWidget *> widgets = widget.findChildren<QWidget *>();
    widgets.prepend(&widget);
    return widgets;
}

void set_style(const QList<QWidget *> &widgets, QStyle *style) {
    for (QWidget *each : widgets) {
        each->setStyle(style);
    }
}

}  // namespace

std::vector<DrawCall> render_with_style(QWidget &widget) {
    return render_with_style(widget, SketchTheme().palette());
}

std::vector<DrawCall> render_with_style(QWidget &widget, const QPalette &palette) {
    HandPaintStyle style;
    widget.setPalette(palette);
    const QList<QWidget *> widgets = widget_and_descendants(widget);
    set_style(widgets, &style);

    RecordingPaintDevice device;
    widget.render(&device);

    // The style dies at the end of this function: no widget may keep it.
    set_style(widgets, nullptr);
    return device.engine().calls();
}

bool has_image(const std::vector<DrawCall> &calls) {
    // drawTiledPixmap falls back to drawPixmap, so every image is a Kind::Pixmap call.
    return std::any_of(calls.begin(), calls.end(),
                       [](const DrawCall &call) { return call.kind == DrawCall::Kind::Pixmap; });
}

}  // namespace hpq::test
