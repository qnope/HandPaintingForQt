#include <hpq/HandPaintEffect.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QFrame>
#include <QImage>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>

#include <memory>

namespace {

const QString FusionStyleName("fusion");
const QString ThemeFontFamily("Architects Daughter");

std::shared_ptr<const hpq::Theme> sketch_theme() {
    return std::make_shared<const hpq::SketchTheme>();
}

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>();
}

TEST(ApplyThemeTest, SetsPaletteFontFusionStyleAndEffect) {
    QMainWindow window;
    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    auto *button = new QPushButton("Button", central);
    auto *check_box = new QCheckBox("Check", central);
    layout->addWidget(button);
    layout->addWidget(check_box);
    window.setCentralWidget(central);
    const auto theme = sketch_theme();

    hpq::apply_theme(window, theme, rough_geometry_engine());

    EXPECT_EQ(window.palette(), theme->palette());
    EXPECT_EQ(window.font().family(), ThemeFontFamily);
    EXPECT_EQ(window.style()->name(), FusionStyleName);
    EXPECT_EQ(button->style()->name(), FusionStyleName);
    EXPECT_EQ(check_box->style()->name(), FusionStyleName);
    EXPECT_NE(qobject_cast<hpq::HandPaintEffect *>(window.graphicsEffect()), nullptr);
}

// A 200x150 window holding a box frame, with the Fusion style and the Sketch palette.
class FramedWindow {
public:
    explicit FramedWindow(const hpq::Theme &theme)
        : m_style(QStyleFactory::create("Fusion")) {
        auto *frame = new QFrame(&m_window);
        frame->setFrameShape(QFrame::Box);
        frame->setLineWidth(2);
        m_window.setCentralWidget(frame);
        m_window.setStyle(m_style.get());
        frame->setStyle(m_style.get());
        m_window.setPalette(theme.palette());
        m_window.resize(200, 150);
    }

    QMainWindow &window() { return m_window; }

private:
    std::unique_ptr<QStyle> m_style;
    QMainWindow m_window;
};

TEST(ApplyThemeTest, ThemedGrabDiffersFromPlainGrab) {
    const auto theme = sketch_theme();
    FramedWindow plain(*theme);
    FramedWindow themed(*theme);

    hpq::apply_theme(themed.window(), theme, rough_geometry_engine());

    const QImage plain_image = plain.window().grab().toImage();
    const QImage themed_image = themed.window().grab().toImage();
    ASSERT_FALSE(plain_image.isNull());
    ASSERT_FALSE(themed_image.isNull());
    EXPECT_NE(plain_image, themed_image);
}

// The number of pixels of `image` inside `rect` darker than a light gray.
int dark_pixel_count(const QImage &image, const QRect &rect) {
    constexpr int MaxDarkLightness = 128;
    int count = 0;
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        for (int x = rect.left(); x <= rect.right(); ++x) {
            if (image.pixelColor(x, y).lightness() < MaxDarkLightness) {
                ++count;
            }
        }
    }
    return count;
}

// A child widget away from the parent origin clips its drawing to its own area: the clip must
// follow the child's offset, or what the child draws inside it (here, the text) is lost.
TEST(ApplyThemeTest, ChildAtAnOffsetKeepsWhatItDrawsInsideItsClip) {
    QWidget parent;
    parent.resize(240, 120);
    auto *line_edit = new QLineEdit("WWWWWWWW", &parent);
    line_edit->setGeometry(40, 40, 160, 30);

    hpq::apply_theme(parent, sketch_theme(), rough_geometry_engine());

    const QImage image = parent.grab().toImage();
    const qreal ratio = image.devicePixelRatio();
    const QRect text_area = line_edit->geometry().adjusted(6, 6, -6, -6);
    const QRect device_text_area(text_area.topLeft() * ratio, text_area.size() * ratio);
    EXPECT_GT(dark_pixel_count(image, device_text_area), 20);
}

}  // namespace
