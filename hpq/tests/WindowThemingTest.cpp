#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/HighlighterTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theming.h>
#include <hpq/WindowTheming.h>

#include <gtest/gtest.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QHelpEvent>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdint>
#include <memory>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed);
}

bool has_hand_paint_effect(const QWidget &widget) {
    return qobject_cast<hpq::HandPaintEffect *>(widget.graphicsEffect()) != nullptr;
}

// True when `window` has the look of `theme`: its palette, its font, a HandPaintStyle on it and
// on its descendants, and a HandPaintEffect of its own.
bool has_look_of(const QWidget &window, const hpq::Theme &theme) {
    if (!has_hand_paint_effect(window) || window.palette() != theme.palette() ||
        window.font().family() != theme.font().family() || !qobject_cast<hpq::HandPaintStyle *>(window.style())) {
        return false;
    }
    for (const QWidget *descendant : window.findChildren<QWidget *>()) {
        if (descendant->window() == &window && descendant->style() != window.style()) {
            return false;
        }
    }
    return true;
}

// True when `window` has none of the look of a theme.
bool has_no_look(const QWidget &window) {
    return window.graphicsEffect() == nullptr && !window.testAttribute(Qt::WA_SetPalette) &&
           !window.testAttribute(Qt::WA_SetFont) && !qobject_cast<hpq::HandPaintStyle *>(window.style());
}

// A shown window holding a button, with a menu that the button opens.
class WindowWithMenu {
public:
    WindowWithMenu() : m_button(new QPushButton("Button", &m_window)), m_menu(new QMenu(m_button)) {
        m_menu->addAction("First entry");
        m_menu->addAction("Second entry");
        m_window.resize(200, 100);
        m_window.show();
    }

    ~WindowWithMenu() { m_menu->hide(); }

    QWidget &window() { return m_window; }
    QPushButton &button() { return *m_button; }

    QMenu &open_menu() {
        m_menu->hide();
        m_menu->popup(m_button->mapToGlobal(QPoint(0, m_button->height())));
        QApplication::processEvents();
        return *m_menu;
    }

private:
    QWidget m_window;
    QPushButton *m_button;
    QMenu *m_menu;
};

// The color of a menu inside its frame, away from its entries.
QColor menu_background(QMenu &menu) {
    const QImage image = menu.grab().toImage();
    return image.pixelColor(image.width() / 2, image.height() - 3 * image.devicePixelRatio());
}

// The tooltip shown, if one is.
QWidget *shown_tool_tip() {
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (widget->windowType() == Qt::ToolTip && widget->isVisible()) {
            return widget;
        }
    }
    return nullptr;
}

// Asks `widget` for its tooltip, as the application does when the mouse rests on it.
void ask_for_tool_tip(QWidget &widget) {
    const QPoint position = widget.rect().center();
    QHelpEvent event(QEvent::ToolTip, position, widget.mapToGlobal(position));
    QApplication::sendEvent(&widget, &event);
}

void hide_tool_tip() {
    QToolTip::hideText();
    QApplication::processEvents();
}

// A WindowTheming of `theme` installed on the application while it lives.
class InstalledWindowTheming {
public:
    explicit InstalledWindowTheming(const hpq::Theme &theme) : m_window_theming(theme, rough_geometry_engine()) {
        QApplication::instance()->installEventFilter(&m_window_theming);
    }
    ~InstalledWindowTheming() { QApplication::instance()->removeEventFilter(&m_window_theming); }
    InstalledWindowTheming(const InstalledWindowTheming &) = delete;
    InstalledWindowTheming &operator=(const InstalledWindowTheming &) = delete;

private:
    hpq::WindowTheming m_window_theming;
};

}  // namespace

// Without a WindowTheming, the application leaves its windows as they are: the menu has only
// the style that apply_theme gives to every descendant.
TEST(WindowThemingTest, WithoutItAMenuHasNeitherThePaletteNorTheEffectOfTheTheme) {
    WindowWithMenu window_with_menu;
    hpq::apply_theme(window_with_menu.window(), hpq::slate_theme(), rough_geometry_engine());

    const QMenu &menu = window_with_menu.open_menu();

    EXPECT_EQ(menu.graphicsEffect(), nullptr);
    EXPECT_FALSE(menu.testAttribute(Qt::WA_SetPalette));
}

TEST(WindowThemingTest, MenuTakesTheTheme) {
    const hpq::Theme theme = hpq::slate_theme();
    InstalledWindowTheming window_theming(theme);
    WindowWithMenu window_with_menu;

    EXPECT_TRUE(has_look_of(window_with_menu.open_menu(), theme));
}

// A menu that is not themed is light; the slate is a dark board.
TEST(WindowThemingTest, MenuIsDrawnInTheColorsOfTheTheme) {
    constexpr int MaxDarkLightness = 100;
    WindowWithMenu window_with_menu;
    EXPECT_GT(menu_background(window_with_menu.open_menu()).lightness(), MaxDarkLightness);

    InstalledWindowTheming window_theming(hpq::slate_theme());

    EXPECT_LT(menu_background(window_with_menu.open_menu()).lightness(), MaxDarkLightness);
}

// A window without parent, as the menu that a context menu event often opens.
TEST(WindowThemingTest, MenuWithoutParentTakesTheTheme) {
    const hpq::Theme theme = hpq::slate_theme();
    InstalledWindowTheming window_theming(theme);
    QMenu menu;
    menu.addAction("Entry");

    menu.popup(QPoint(10, 10));
    QApplication::processEvents();

    EXPECT_TRUE(has_look_of(menu, theme));
    menu.hide();
}

TEST(WindowThemingTest, MenuShownAgainTakesTheThemeOfANewWindowTheming) {
    WindowWithMenu window_with_menu;
    {
        InstalledWindowTheming window_theming(hpq::sketch_theme());
        window_with_menu.open_menu();
    }

    InstalledWindowTheming window_theming(hpq::slate_theme());

    EXPECT_TRUE(has_look_of(window_with_menu.open_menu(), hpq::slate_theme()));
}

// A main window is themed by the application, with apply_theme.
TEST(WindowThemingTest, MainWindowKeepsItsLook) {
    InstalledWindowTheming window_theming(hpq::slate_theme());
    QWidget window;

    window.show();
    QApplication::processEvents();

    EXPECT_TRUE(has_no_look(window));
}

// The tooltip is sized with the font of the theme: its whole text is shown.
TEST(WindowThemingTest, ToolTipTakesTheTheme) {
    const hpq::Theme theme = hpq::slate_theme();
    InstalledWindowTheming window_theming(theme);
    WindowWithMenu window_with_menu;
    window_with_menu.button().setToolTip("A tooltip long enough to be cut");

    ask_for_tool_tip(window_with_menu.button());
    const QWidget *tool_tip = shown_tool_tip();

    ASSERT_NE(tool_tip, nullptr);
    EXPECT_TRUE(has_look_of(*tool_tip, theme));
    EXPECT_GE(tool_tip->width(), tool_tip->sizeHint().width());
    EXPECT_GE(tool_tip->height(), tool_tip->sizeHint().height());
    hide_tool_tip();
}

// A dialog holding a label, a combo box and a button that opens a menu.
class Dialog : public QDialog {
public:
    explicit Dialog(QWidget &parent)
        : QDialog(&parent), m_label(new QLabel("A label")), m_combo_box(new QComboBox),
          m_button(new QPushButton("Button")), m_menu(new QMenu(m_button)) {
        m_combo_box->addItems({"First", "Second"});
        m_menu->addAction("Entry");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(m_label);
        layout->addWidget(m_combo_box);
        layout->addWidget(m_button);
    }

    QLabel &label() { return *m_label; }
    QAbstractItemView &list() { return *m_combo_box->view(); }
    QMenu &menu() { return *m_menu; }

private:
    QLabel *m_label;
    QComboBox *m_combo_box;
    QPushButton *m_button;
    QMenu *m_menu;
};

// So do its label, the list of its combo box and the menu it opens.
TEST(WindowThemingTest, DialogTakesTheTheme) {
    const hpq::Theme theme = hpq::highlighter_theme();
    InstalledWindowTheming window_theming(theme);
    QWidget window;

    Dialog dialog(window);
    dialog.show();
    QApplication::processEvents();

    EXPECT_TRUE(has_look_of(dialog, theme));
    EXPECT_EQ(dialog.label().property(hpq::HandPaintStyle::LabelMarkerProperty), QVariant(theme.label_marker()));
    EXPECT_EQ(dialog.list().palette(), theme.palette());
    EXPECT_TRUE(has_hand_paint_effect(*dialog.list().window()));
    EXPECT_TRUE(has_look_of(dialog.menu(), theme));
}
