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
    // Applies the theme of the comboBox item at index to the menu bar, central widget and status bar;
    // the item "None" removes the effects.
    void apply_theme(int index);

    std::unique_ptr<Ui::MainWindow> m_ui;
};

} // namespace hpq::example
