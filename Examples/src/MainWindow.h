#pragma once

#include <hpq/WindowTheming.h>

#include <QMainWindow>
#include <memory>
#include <optional>

#include "FwdDecls.h"

namespace hpq::example {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // Applies the theme of the entry of comboBox, with hatched fills when checkBox is checked,
    // to the window and, with m_window_theming, to the menus, dialogs and tooltips of the
    // application. Removes the theme when comboBox has no current entry.
    void update_theme();

    // Opens a message box about the library, which takes the theme of the window.
    void show_about();

    std::unique_ptr<Ui::MainWindow> m_ui;
    // Installed on the application while the window has a theme.
    std::optional<hpq::WindowTheming> m_window_theming;
};

} // namespace hpq::example
