#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QFrame>
#include <QImage>
#include <QLineEdit>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QTableView>
#include <QVBoxLayout>
#include <QWindow>

#include <memory>
#include <string>
#include <vector>

#include <cstdint>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

const QString ThemeFontFamily("Architects Daughter");

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed);
}

TEST(ApplyThemeTest, SetsPaletteFontHandPaintStyleAndEffect) {
    QMainWindow window;
    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    auto *button = new QPushButton("Button", central);
    auto *check_box = new QCheckBox("Check", central);
    layout->addWidget(button);
    layout->addWidget(check_box);
    window.setCentralWidget(central);
    const hpq::Theme theme = hpq::sketch_theme();

    hpq::apply_theme(window, theme, rough_geometry_engine());

    EXPECT_EQ(window.palette(), theme.palette());
    EXPECT_EQ(window.font().family(), ThemeFontFamily);
    const auto *style = qobject_cast<hpq::HandPaintStyle *>(window.style());
    ASSERT_NE(style, nullptr);
    for (QWidget *descendant : window.findChildren<QWidget *>()) {
        EXPECT_EQ(descendant->style(), style) << descendant->metaObject()->className();
    }
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
    const hpq::Theme theme = hpq::sketch_theme();
    FramedWindow plain(theme);
    FramedWindow themed(theme);

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

    hpq::apply_theme(parent, hpq::sketch_theme(), rough_geometry_engine());

    const QImage image = parent.grab().toImage();
    const qreal ratio = image.devicePixelRatio();
    const QRect text_area = line_edit->geometry().adjusted(6, 6, -6, -6);
    const QRect device_text_area(text_area.topLeft() * ratio, text_area.size() * ratio);
    EXPECT_GT(dark_pixel_count(image, device_text_area), 20);
}

// Messages from the platform and not from hpq: the offscreen platform's font and window
// messages. Any other warning or critical message fails the paint test.
const std::vector<QString> IgnoredCategoryPrefixes{"qt.qpa."};
const std::vector<QString> IgnoredMessages{"This plugin does not support propagateSizeHints()"};

QString category_of(const QMessageLogContext &context) {
    return context.category != nullptr ? QString(context.category) : QString();
}

bool is_ignored(const QMessageLogContext &context, const QString &message) {
    const QString category = category_of(context);
    for (const QString &prefix : IgnoredCategoryPrefixes) {
        if (category.startsWith(prefix)) {
            return true;
        }
    }
    for (const QString &ignored : IgnoredMessages) {
        if (message.contains(ignored)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> collected_warnings;

void collect_warnings(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    const bool is_warning = type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg;
    if (is_warning && !is_ignored(context, message)) {
        collected_warnings.push_back((category_of(context) + ": " + message).toStdString());
    }
}

// Collects the warnings, not from the platform, sent while it lives.
class WarningCollector {
public:
    WarningCollector() : m_previous_handler(qInstallMessageHandler(collect_warnings)) {
        collected_warnings.clear();
    }
    ~WarningCollector() { qInstallMessageHandler(m_previous_handler); }
    WarningCollector(const WarningCollector &) = delete;
    WarningCollector &operator=(const WarningCollector &) = delete;

    const std::vector<std::string> &warnings() const { return collected_warnings; }

private:
    QtMessageHandler m_previous_handler;
};

// Processes events until the window of `widget` is exposed, or a time limit; true if exposed.
bool wait_for_window_exposed(QWidget &widget) {
    constexpr int TimeoutMs = 5000;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < TimeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        const QWindow *window = widget.windowHandle();
        if (window != nullptr && window->isExposed()) {
            return true;
        }
    }
    return false;
}

TEST(ApplyThemeTest, ThemedWindowPaintsWithoutWarning) {
    WarningCollector collector;
    QMainWindow window;
    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(new QPushButton("Button", central));
    layout->addWidget(new QCheckBox("Check", central));
    layout->addWidget(new QLineEdit("Text", central));
    auto *model = new QStandardItemModel(3, 2, central);
    model->setData(model->index(0, 0), "Cell");
    auto *table = new QTableView(central);
    table->setModel(model);
    layout->addWidget(table);
    auto *scene = new QGraphicsScene(central);
    scene->addRect(0, 0, 50, 30);
    scene->addText("Scene");
    layout->addWidget(new QGraphicsView(scene, central));
    window.setCentralWidget(central);
    window.resize(400, 500);
    hpq::apply_theme(window, hpq::sketch_theme(), rough_geometry_engine());

    window.show();
    ASSERT_TRUE(wait_for_window_exposed(window));
    window.repaint();
    const QImage image = window.grab().toImage();
    QCoreApplication::processEvents();

    EXPECT_FALSE(image.isNull());
    EXPECT_TRUE(collector.warnings().empty()) << ::testing::PrintToString(collector.warnings());
}

// Sends a left button press then release at the center of `widget` to its top-level window,
// so that the application finds the receiver from the position, as for a real click.
void click_center(QWidget &widget) {
    QWidget &top_level = *widget.window();
    const QPointF window_position = widget.mapTo(&top_level, widget.rect().center());
    const QPointF global_position = widget.mapToGlobal(widget.rect().center());
    QMouseEvent press(QEvent::MouseButtonPress, window_position, window_position,
                      global_position, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(top_level.windowHandle(), &press);
    QMouseEvent release(QEvent::MouseButtonRelease, window_position, window_position,
                        global_position, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(top_level.windowHandle(), &release);
}

TEST(ApplyThemeTest, ThemedWindowChildGetsMouseClick) {
    QMainWindow window;
    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    auto *check_box = new QCheckBox("Check", central);
    // At its size hint, so that its center is on its click area.
    layout->addWidget(check_box, 0, Qt::AlignLeft | Qt::AlignTop);
    window.setCentralWidget(central);
    window.resize(200, 100);
    hpq::apply_theme(window, hpq::sketch_theme(), rough_geometry_engine());
    window.show();
    ASSERT_TRUE(wait_for_window_exposed(window));
    ASSERT_FALSE(check_box->isChecked());

    click_center(*check_box);

    EXPECT_TRUE(check_box->isChecked());
}

}  // namespace
