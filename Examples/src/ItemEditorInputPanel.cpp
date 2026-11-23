#include "ItemEditorInputPanel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QInputMethod>
#include <QMouseEvent>
#include <QWidget>

#include <utility>

namespace hpq::example {

namespace {
// True when the widget with the focus is an editor of the view of `viewport` that takes text:
// the editors of the items are children of the viewport.
bool has_focused_text_editor(const QWidget &viewport)
{
    const QWidget *focus_widget = QApplication::focusWidget();
    return focus_widget && focus_widget != &viewport && viewport.isAncestorOf(focus_widget)
        && focus_widget->testAttribute(Qt::WA_InputMethodEnabled);
}
} // namespace

ItemEditorInputPanel::ItemEditorInputPanel(QAbstractItemView &view, Request request)
    : QObject(&view)
    , m_request(std::move(request))
{
    view.viewport()->installEventFilter(this);
}

void ItemEditorInputPanel::show_input_panel()
{
    QGuiApplication::inputMethod()->show();
}

bool ItemEditorInputPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease && watched->isWidgetType()) {
        const auto *mouse_event = static_cast<const QMouseEvent *>(event);
        const auto *viewport = static_cast<const QWidget *>(watched);
        if (mouse_event->button() == Qt::LeftButton && has_focused_text_editor(*viewport))
            m_request();
    }
    return QObject::eventFilter(watched, event);
}

} // namespace hpq::example
