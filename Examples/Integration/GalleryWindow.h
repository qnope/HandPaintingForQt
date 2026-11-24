#pragma once

#include <hpq/GeometryEngine.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/WindowTheming.h>

#include <QWidget>

#include <memory>
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
    // `geometry_engine` makes the shapes rough. Each RoughGeometryEngine has its own hand;
    // one with a fixed base seed draws the same window every time.
    explicit GalleryWindow(std::shared_ptr<const hpq::GeometryEngine> geometry_engine =
                               std::make_shared<const hpq::RoughGeometryEngine>(),
                           QWidget *parent = nullptr);

private:
    // Gives the window, and the menus, dialogs and tooltips of the application, the theme
    // chosen in m_theme_box.
    void update_theme();

    std::shared_ptr<const hpq::GeometryEngine> m_geometry_engine;
    QComboBox *m_theme_box;
    QCheckBox *m_hachure_box;
    // Themes the windows of their own of the application (menus, dialogs, tooltips).
    std::optional<hpq::WindowTheming> m_window_theming;
};

}  // namespace hpq::gallery
