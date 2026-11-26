#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

#include <QObject>

class QWidget;

namespace hpq {

// Gives `window` the look of `theme`: its palette and font on the window, one HandPaintStyle
// on the window and its existing descendant widgets, and a HandPaintEffect on the window.
// Its existing labels get the marker of the theme, if it has one.
// The lists that its combo boxes open are windows of their own: each gets the palette and a
// HandPaintEffect too. Menus, dialogs and tooltips, windows of their own as well, get a theme
// from a WindowTheming that the application installs.
HPQ_EXPORT void apply_theme(QWidget &window, const Theme &theme);

// Removes the HandPaintEffect of `window` and of the lists of its combo boxes, takes the
// markers away from its labels, and gives back the palettes, font and styles they had before
// the first apply_theme. Does nothing on a window that was never themed.
HPQ_EXPORT void remove_theme(QWidget &window);

// An event filter that gives a theme to the menus, the dialogs and the tooltips of an
// application. They are windows of their own, which apply_theme on a widget does not reach.
// The application installs it if it wants them themed:
//
//     hpq::WindowTheming window_theming(theme);
//     application.installEventFilter(&window_theming);
//
// Each window that is not a main window (Qt::Window) gets apply_theme when it is polished,
// before Qt computes its size with its font, and each time it is shown, so that a window themed
// by a WindowTheming installed before takes the theme of this one.
class HPQ_EXPORT WindowTheming : public QObject {
    Q_OBJECT

public:
    explicit WindowTheming(Theme theme, QObject *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Theme m_theme;
};

}  // namespace hpq
