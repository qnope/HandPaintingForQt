#pragma once

#include <hpq/Theming.h>

#include <QWidget>

#include <cstdint>
#include <optional>

class QCheckBox;
class QComboBox;

namespace hpq::gallery {

// A window of common Qt widgets and of shapes painted with QPainter, drawn by hand with the
// theme chosen in its "Theme" combo box ("themeBox"), with hatched fills when its "Hachures"
// check box ("hachureBox") is checked.
class GalleryWindow : public QWidget {
    Q_OBJECT

public:
    // `seed` is the hand that draws the window (hpq::RoughOptions::seed): the same seed draws
    // the same window every time.
    explicit GalleryWindow(std::uint32_t seed = 0, QWidget *parent = nullptr);

private:
    // Gives the window, and the menus, dialogs and tooltips of the application, the theme
    // chosen in m_theme_box.
    void update_theme();

    std::uint32_t m_seed;
    QComboBox *m_theme_box;
    QCheckBox *m_hachure_box;
    // Themes the windows of their own of the application (menus, dialogs, tooltips).
    std::optional<hpq::WindowTheming> m_window_theming;
};

}  // namespace hpq::gallery
