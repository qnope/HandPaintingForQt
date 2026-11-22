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
    // Applies the Sketch theme for the "Sketch" entry of comboBox, with hatched fills when
    // checkBox is checked, and removes the theme otherwise.
    void update_theme();

    std::unique_ptr<Ui::MainWindow> m_ui;
};

} // namespace hpq::example
