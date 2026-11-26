#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStyle>
#include <QStyleFactory>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <memory>

namespace {

// Builds the central widget of a window and returns the widget the test looks at.
using Builder = std::function<QWidget *(QWidget &central)>;

// A grab of a window built by a Builder, and the area of the looked-at widget in it.
struct Grab {
    QImage image;
    QRect widget_area;  // In image pixels.
};

// Builds a 400x260 window with `build`, gives it the Fusion style, the Sketch palette and
// font, applies the theme when `themed`, and grabs it.
Grab grab_window(const Builder &build, bool themed) {
    const hpq::Theme theme = hpq::sketch_theme();
    std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));
    QMainWindow window;
    auto *central = new QWidget;
    QWidget *looked_at = build(*central);
    window.setCentralWidget(central);
    window.resize(400, 260);
    if (themed) {
        hpq::apply_theme(window, theme);
    } else {
        window.setPalette(theme.palette);
        window.setFont(theme.font);
        window.setStyle(style.get());
        for (QWidget *descendant : window.findChildren<QWidget *>()) {
            descendant->setStyle(style.get());
        }
    }
    Grab grab;
    grab.image = window.grab().toImage();
    const qreal ratio = grab.image.devicePixelRatio();
    const QRect area(looked_at->mapTo(&window, QPoint(0, 0)), looked_at->size());
    grab.widget_area = QRect(area.topLeft() * ratio, area.size() * ratio);
    return grab;
}

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

// The share (0..1) of the columns of `band` that hold a line clearly darker than the window
// color: a line drawn across the band, even wobbly, covers every column. A smoothed line that
// falls between two rows of pixels spreads its ink over both, lighter in each: the darkenings
// of two pixels one above the other add up.
double covered_column_share(const QImage &image, const QRect &band) {
    const int window_lightness = hpq::sketch_theme().palette.color(QPalette::Window).lightness();
    constexpr int MinDarkening = 30;
    const auto darkening = [&](int x, int y) {
        return std::max(0, window_lightness - image.pixelColor(x, y).lightness());
    };
    int covered = 0;
    for (int x = band.left(); x <= band.right(); ++x) {
        for (int y = band.top(); y < band.bottom(); ++y) {
            if (darkening(x, y) + darkening(x, y + 1) > MinDarkening) {
                ++covered;
                break;
            }
        }
    }
    return static_cast<double>(covered) / band.width();
}

// A band around the horizontal edge at `y`, across `area` except its rounded corners.
QRect edge_band(const QRect &area, int y) {
    constexpr int HalfHeight = 3;
    constexpr int Margin = 6;
    return QRect(area.left() + Margin, y - HalfHeight, area.width() - 2 * Margin, 2 * HalfHeight + 1);
}

QWidget *group_box_in_tab(QWidget &central) {
    auto *tabs = new QTabWidget;
    auto *page = new QWidget;
    auto *group_box = new QGroupBox("WWWWWWWW WWWWWWWW");
    (new QVBoxLayout(group_box))->addWidget(new QLabel("label"));
    (new QVBoxLayout(page))->addWidget(group_box);
    tabs->addTab(page, "Tab");
    (new QVBoxLayout(&central))->addWidget(tabs);
    return group_box;
}

TEST(NestedThemeTest, GroupBoxTitleInATabPageIsDrawn) {
    const Grab plain = grab_window(group_box_in_tab, false);
    const Grab themed = grab_window(group_box_in_tab, true);

    // The left part of the title, under the selected tab.
    const QSize TitleStart(80, 20);
    const QRect plain_title(plain.widget_area.topLeft(), TitleStart);
    const QRect themed_title(themed.widget_area.topLeft(), TitleStart);
    const int plain_count = dark_pixel_count(plain.image, plain_title);
    ASSERT_GT(plain_count, 50);
    EXPECT_GT(dark_pixel_count(themed.image, themed_title), plain_count * 4 / 5);
}

// A spin box between other widgets, in a group box in a splitter.
QWidget *spin_box_in_group(QWidget &central) {
    auto *group_box = new QGroupBox("Group");
    auto *group_layout = new QVBoxLayout(group_box);
    group_layout->addWidget(new QLabel("Label"));
    group_layout->addWidget(new QLineEdit("Edit"));
    auto *combo = new QComboBox;
    combo->addItem("Combo");
    group_layout->addWidget(combo);
    auto *spin = new QSpinBox;
    spin->setValue(41);
    group_layout->addWidget(spin);
    auto *check_box = new QCheckBox("Check");
    check_box->setChecked(true);
    group_layout->addWidget(check_box);
    group_layout->addWidget(new QRadioButton("Radio"));
    group_layout->addWidget(new QSlider(Qt::Horizontal));
    group_layout->addWidget(new QPushButton("Button"));
    auto *splitter = new QSplitter;
    splitter->addWidget(group_box);
    splitter->addWidget(new QLabel("right"));
    (new QVBoxLayout(&central))->addWidget(splitter);
    return spin;
}

TEST(NestedThemeTest, SpinBoxFrameHasNoHoles) {
    const Grab themed = grab_window(spin_box_in_group, true);
    const QRect area = themed.widget_area;

    EXPECT_GT(covered_column_share(themed.image, edge_band(area, area.top())), 0.99);
    EXPECT_GT(covered_column_share(themed.image, edge_band(area, area.bottom())), 0.99);
}

QWidget *list_in_second_tab(QWidget &central) {
    auto *tabs = new QTabWidget;
    tabs->addTab(new QLabel("first"), "First tab");
    auto *page = new QWidget;
    auto *list = new QListWidget;
    list->addItems({"one", "two"});
    (new QVBoxLayout(page))->addWidget(list);
    tabs->addTab(page, "Second tab");
    tabs->setCurrentIndex(1);
    (new QVBoxLayout(&central))->addWidget(tabs);
    return list;
}

TEST(NestedThemeTest, SelectedTabFillDoesNotCoverTheListBelow) {
    const Grab themed = grab_window(list_in_second_tab, true);
    const QRect area = themed.widget_area;

    EXPECT_GT(covered_column_share(themed.image, edge_band(area, area.top())), 0.99);
}

}  // namespace

namespace {

// The number of pixels of `image` that are not fully opaque.
int transparent_pixel_count(const QImage &image) {
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() < 255) {
                ++count;
            }
        }
    }
    return count;
}

// A group box beside a tab widget whose current page holds a list: the window background is
// then painted as several adjacent rectangles around the opaque list.
QWidget *group_box_beside_list_in_tab(QWidget &central) {
    auto *splitter = new QSplitter;
    auto *group_box = new QGroupBox("In splitter");
    (new QVBoxLayout(group_box))->addWidget(new QLabel("Label"));
    splitter->addWidget(group_box);
    auto *tabs = new QTabWidget;
    tabs->addTab(new QLabel("first"), "First tab");
    auto *page = new QWidget;
    auto *list = new QListWidget;
    list->addItems({"one", "two"});
    (new QVBoxLayout(page))->addWidget(list);
    tabs->addTab(page, "Second tab");
    tabs->setCurrentIndex(1);
    splitter->addWidget(tabs);
    (new QVBoxLayout(&central))->addWidget(splitter);
    return group_box;
}

TEST(NestedThemeTest, BackgroundLeavesNoUnpaintedSeam) {
    const Grab plain = grab_window(group_box_beside_list_in_tab, false);
    const Grab themed = grab_window(group_box_beside_list_in_tab, true);

    ASSERT_EQ(transparent_pixel_count(plain.image), 0);
    EXPECT_EQ(transparent_pixel_count(themed.image), 0);
}

}  // namespace
