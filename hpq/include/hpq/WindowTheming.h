#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>

#include <QObject>
#include <QPointer>

class QWidget;

namespace hpq {

// An event filter that gives the menus, the dialogs and the tooltips of an application the theme
// of the widget that opens them. They are windows of their own, which apply_theme on that widget
// does not reach. The application decides whether they follow it, by installing the filter:
//
//     hpq::WindowTheming window_theming;
//     application.installEventFilter(&window_theming);
//
// A window is opened by its parent widget, and a tooltip without parent (as under Windows) by the
// widget that asked for it. When it is polished, before Qt computes its size, and each time it
// is shown, the window gets the theme of the nearest of its opener and the ancestors of its opener
// given one with apply_theme, or loses its theme if none has one. A window without opener, such
// as the main window, is left as it is.
class HPQ_EXPORT WindowTheming : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void follow_opener(QWidget &window) const;

    // The last widget that got a tooltip event: a tooltip is made while that widget handles it.
    QPointer<QWidget> m_tool_tip_asker;
};

}  // namespace hpq
