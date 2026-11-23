#include <hpq/HandPaintEffect.h>
#include <hpq/Theming.h>
#include <hpq/WindowTheming.h>

#include <QEvent>
#include <QWidget>

namespace hpq {
namespace {

// The effect of the nearest of `widget` and its ancestors given a theme, or null.
const HandPaintEffect *theme_effect(const QWidget &widget) {
    for (const QWidget *ancestor = &widget; ancestor; ancestor = ancestor->parentWidget()) {
        if (const auto *effect = qobject_cast<const HandPaintEffect *>(ancestor->graphicsEffect())) {
            return effect;
        }
    }
    return nullptr;
}

}  // namespace

bool WindowTheming::eventFilter(QObject *watched, QEvent *event) {
    if (!watched->isWidgetType()) {
        return false;
    }
    auto &widget = static_cast<QWidget &>(*watched);
    if (event->type() == QEvent::ToolTip) {
        m_tool_tip_asker = &widget;
    } else if ((event->type() == QEvent::Polish || event->type() == QEvent::Show) && widget.isWindow()) {
        follow_opener(widget);
    }
    return false;
}

void WindowTheming::follow_opener(QWidget &window) const {
    const QWidget *opener = window.parentWidget();
    if (!opener && window.windowType() == Qt::ToolTip) {
        opener = m_tool_tip_asker;
    }
    if (!opener) {
        return;
    }
    if (const HandPaintEffect *effect = theme_effect(*opener)) {
        apply_theme(window, effect->theme(), effect->geometry_engine());
    } else {
        remove_theme(window);
    }
}

}  // namespace hpq
