#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>
#include <hpq/Theme.h>

#include <QObject>

#include <memory>

namespace hpq {

// An event filter that gives a theme to the menus, the dialogs and the tooltips of an
// application. They are windows of their own, which apply_theme on a widget does not reach.
// The application installs it if it wants them themed:
//
//     hpq::WindowTheming window_theming(theme, geometry_engine);
//     application.installEventFilter(&window_theming);
//
// Each window that is not a main window (Qt::Window) gets apply_theme when it is polished,
// before Qt computes its size with its font, and each time it is shown, so that a window themed
// by a WindowTheming installed before takes the theme of this one.
class HPQ_EXPORT WindowTheming : public QObject {
    Q_OBJECT

public:
    WindowTheming(Theme theme, std::shared_ptr<const GeometryEngine> geometry_engine,
                  QObject *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Theme m_theme;
    std::shared_ptr<const GeometryEngine> m_geometry_engine;
};

}  // namespace hpq
