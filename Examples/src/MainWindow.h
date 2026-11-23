#pragma once

#include <QMainWindow>
#include <memory>

#include "FwdDecls.h"

namespace hpq::example {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // Applies the theme of the entry of comboBox (Sketch, China Ink, or slate for "Black slate
    // and chalk"), with hatched fills when checkBox is checked. Removes the theme when comboBox
    // has no current entry.
    void update_theme();

    std::unique_ptr<Ui::MainWindow> m_ui;
};

} // namespace hpq::example
