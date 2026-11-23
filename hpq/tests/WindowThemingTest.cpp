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

// A WindowTheming installed on the application while the test runs.
class WindowThemingTest : public testing::Test {
protected:
    WindowThemingTest() { QApplication::instance()->installEventFilter(&m_window_theming); }
    ~WindowThemingTest() override { QApplication::instance()->removeEventFilter(&m_window_theming); }

    hpq::WindowTheming m_window_theming;
};

}  // namespace

// Without a WindowTheming, the application leaves its windows as they are: the menu has only
// the style that apply_theme gives to every descendant.
TEST(WithoutWindowThemingTest, MenuHasNeitherThePaletteNorTheEffectOfTheTheme) {
    WindowWithMenu window_with_menu;
    hpq::apply_theme(window_with_menu.window(), hpq::slate_theme(), rough_geometry_engine());

    const QMenu &menu = window_with_menu.open_menu();

    EXPECT_EQ(menu.graphicsEffect(), nullptr);
    EXPECT_FALSE(menu.testAttribute(Qt::WA_SetPalette));
}

TEST_F(WindowThemingTest, MenuTakesTheThemeOfTheWidgetThatOpensIt) {
    WindowWithMenu window_with_menu;
    const hpq::Theme theme = hpq::slate_theme();
    hpq::apply_theme(window_with_menu.window(), theme, rough_geometry_engine());

    EXPECT_TRUE(has_look_of(window_with_menu.open_menu(), theme));
}

// A menu that is not themed is light; the slate is a dark board.
TEST_F(WindowThemingTest, MenuIsDrawnInTheColorsOfTheTheme) {
    constexpr int MaxDarkLightness = 100;
    WindowWithMenu plain;
    WindowWithMenu themed;
    hpq::apply_theme(themed.window(), hpq::slate_theme(), rough_geometry_engine());

    EXPECT_GT(menu_background(plain.open_menu()).lightness(), MaxDarkLightness);
    EXPECT_LT(menu_background(themed.open_menu()).lightness(), MaxDarkLightness);
}

TEST_F(WindowThemingTest, MenuOfAnUnthemedWindowKeepsItsLook) {
    WindowWithMenu themed;
    WindowWithMenu plain;
    hpq::apply_theme(themed.window(), hpq::slate_theme(), rough_geometry_engine());

    EXPECT_TRUE(has_no_look(plain.open_menu()));
}

TEST_F(WindowThemingTest, MenuShownAgainTakesTheNewThemeOfItsOpener) {
    WindowWithMenu window_with_menu;
    hpq::apply_theme(window_with_menu.window(), hpq::sketch_theme(), rough_geometry_engine());
    window_with_menu.open_menu();

    hpq::apply_theme(window_with_menu.window(), hpq::slate_theme(), rough_geometry_engine());

    EXPECT_TRUE(has_look_of(window_with_menu.open_menu(), hpq::slate_theme()));
}

TEST_F(WindowThemingTest, MenuShownAgainLosesTheThemeRemovedFromItsOpener) {
    WindowWithMenu window_with_menu;
    hpq::apply_theme(window_with_menu.window(), hpq::slate_theme(), rough_geometry_engine());
    window_with_menu.open_menu();

    hpq::remove_theme(window_with_menu.window());

    EXPECT_TRUE(has_no_look(window_with_menu.open_menu()));
}

// The theme a window takes is the one of the nearest widget given a theme: the menu of a
// panel given its own theme takes the theme of the panel.
TEST_F(WindowThemingTest, MenuTakesTheThemeOfTheNearestThemedWidget) {
    WindowWithMenu outer;
    auto *panel = new QWidget(&outer.window());
    auto *inner_menu = new QMenu(new QPushButton(panel));
    inner_menu->addAction("Entry");
    hpq::apply_theme(outer.window(), hpq::sketch_theme(), rough_geometry_engine());
    hpq::apply_theme(*panel, hpq::slate_theme(), rough_geometry_engine());

    inner_menu->ensurePolished();

    EXPECT_TRUE(has_look_of(*inner_menu, hpq::slate_theme()));
    EXPECT_TRUE(has_look_of(outer.open_menu(), hpq::sketch_theme()));
}

// The tooltip is sized with the font of the theme: its whole text is shown.
TEST_F(WindowThemingTest, ToolTipTakesTheThemeOfTheWidgetThatAsksForIt) {
    WindowWithMenu window_with_menu;
    const hpq::Theme theme = hpq::slate_theme();
    window_with_menu.button().setToolTip("A tooltip long enough to be cut");
    hpq::apply_theme(window_with_menu.window(), theme, rough_geometry_engine());

    ask_for_tool_tip(window_with_menu.button());
    const QWidget *tool_tip = shown_tool_tip();

    ASSERT_NE(tool_tip, nullptr);
    EXPECT_TRUE(has_hand_paint_effect(*tool_tip));
    EXPECT_NE(qobject_cast<hpq::HandPaintStyle *>(tool_tip->style()), nullptr);
    EXPECT_EQ(tool_tip->palette().color(QPalette::ToolTipBase), theme.palette().color(QPalette::ToolTipBase));
    EXPECT_EQ(tool_tip->font().family(), theme.font().family());
    EXPECT_GE(tool_tip->width(), tool_tip->sizeHint().width());
    EXPECT_GE(tool_tip->height(), tool_tip->sizeHint().height());
    hide_tool_tip();
}

// A button that shows its tooltip in a window without parent, as Qt does under Windows.
class ButtonWithParentlessToolTip : public QPushButton {
public:
    using QPushButton::QPushButton;

    const QWidget *tool_tip() const { return m_tool_tip.get(); }

protected:
    bool event(QEvent *event) override {
        if (event->type() != QEvent::ToolTip) {
            return QPushButton::event(event);
        }
        m_tool_tip = std::make_unique<QLabel>("A tooltip", nullptr, Qt::ToolTip);
        m_tool_tip->ensurePolished();
        return true;
    }

private:
    std::unique_ptr<QLabel> m_tool_tip;
};

TEST_F(WindowThemingTest, ToolTipWithoutParentTakesTheThemeOfTheWidgetThatAskedForIt) {
    QWidget window;
    auto *button = new ButtonWithParentlessToolTip("Button", &window);
    const hpq::Theme theme = hpq::slate_theme();
    hpq::apply_theme(window, theme, rough_geometry_engine());

    ask_for_tool_tip(*button);

    ASSERT_NE(button->tool_tip(), nullptr);
    EXPECT_TRUE(has_look_of(*button->tool_tip(), theme));
}

TEST_F(WindowThemingTest, ToolTipOfAnUnthemedWidgetKeepsItsLook) {
    WindowWithMenu themed;
    WindowWithMenu plain;
    plain.button().setToolTip("A tooltip");
    hpq::apply_theme(themed.window(), hpq::slate_theme(), rough_geometry_engine());

    ask_for_tool_tip(plain.button());
    const QWidget *tool_tip = shown_tool_tip();

    ASSERT_NE(tool_tip, nullptr);
    EXPECT_EQ(tool_tip->graphicsEffect(), nullptr);
    EXPECT_EQ(qobject_cast<hpq::HandPaintStyle *>(tool_tip->style()), nullptr);
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

// The windows that the dialog opens take its theme in turn.
TEST_F(WindowThemingTest, DialogTakesTheThemeOfTheWidgetThatOpensIt) {
    QWidget window;
    const hpq::Theme theme = hpq::highlighter_theme();
    hpq::apply_theme(window, theme, rough_geometry_engine());

    Dialog dialog(window);
    dialog.show();
    QApplication::processEvents();

    EXPECT_TRUE(has_look_of(dialog, theme));
    EXPECT_EQ(dialog.label().property(hpq::HandPaintStyle::LabelMarkerProperty), QVariant(theme.label_marker()));
    EXPECT_EQ(dialog.list().palette(), theme.palette());
    EXPECT_TRUE(has_hand_paint_effect(*dialog.list().window()));
    EXPECT_TRUE(has_look_of(dialog.menu(), theme));
}
