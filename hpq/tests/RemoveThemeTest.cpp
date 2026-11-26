#include <hpq/HandPaintEffect.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QFont>
#include <QPalette>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>

#include <memory>

namespace {

// A window with its own "Windows" style and a child button with the application style.
class WindowWithButton {
public:
    WindowWithButton() : m_window_style(QStyleFactory::create("Windows")) {
        m_window.setStyle(m_window_style.get());
        m_button = new QPushButton("Button", &m_window);
    }

    QWidget &window() { return m_window; }
    QPushButton &button() { return *m_button; }

private:
    std::unique_ptr<QStyle> m_window_style;
    QWidget m_window;
    QPushButton *m_button = nullptr;
};

// The palette, font and styles of a WindowWithButton at one moment.
struct LookSnapshot {
    QPalette palette;
    QFont font;
    QStyle *window_style;
    QString window_style_name;
    QStyle *button_style;
    QString button_style_name;
};

LookSnapshot take_snapshot(WindowWithButton &window_with_button) {
    QWidget &window = window_with_button.window();
    QPushButton &button = window_with_button.button();
    return {window.palette(),        window.font(),  window.style(),
            window.style()->name(), button.style(), button.style()->name()};
}

void expect_same_look(const LookSnapshot &actual, const LookSnapshot &expected) {
    EXPECT_EQ(actual.palette, expected.palette);
    EXPECT_EQ(actual.font, expected.font);
    EXPECT_EQ(actual.window_style, expected.window_style);
    EXPECT_EQ(actual.window_style_name, expected.window_style_name);
    EXPECT_EQ(actual.button_style, expected.button_style);
    EXPECT_EQ(actual.button_style_name, expected.button_style_name);
}

TEST(RemoveThemeTest, RestoresEffectPaletteFontAndStyles) {
    WindowWithButton window_with_button;
    const LookSnapshot before = take_snapshot(window_with_button);

    hpq::apply_theme(window_with_button.window(), hpq::sketch_theme());
    hpq::remove_theme(window_with_button.window());

    EXPECT_EQ(window_with_button.window().graphicsEffect(), nullptr);
    expect_same_look(take_snapshot(window_with_button), before);
}

TEST(RemoveThemeTest, ApplyTwiceKeepsOneEffectAndRestoresFirstValues) {
    WindowWithButton window_with_button;
    QWidget &window = window_with_button.window();
    const LookSnapshot before = take_snapshot(window_with_button);

    hpq::apply_theme(window, hpq::sketch_theme());
    hpq::apply_theme(window, hpq::sketch_theme());

    EXPECT_NE(qobject_cast<hpq::HandPaintEffect *>(window.graphicsEffect()), nullptr);
    EXPECT_EQ(window.findChildren<hpq::HandPaintEffect *>().size(), 1);

    hpq::remove_theme(window);

    EXPECT_EQ(window.graphicsEffect(), nullptr);
    expect_same_look(take_snapshot(window_with_button), before);
}

TEST(RemoveThemeTest, RemoveOnUnthemedWindowChangesNothing) {
    WindowWithButton window_with_button;
    const LookSnapshot before = take_snapshot(window_with_button);

    hpq::remove_theme(window_with_button.window());

    EXPECT_EQ(window_with_button.window().graphicsEffect(), nullptr);
    expect_same_look(take_snapshot(window_with_button), before);
}

}  // namespace
