#include <hpq/ChinaInkTheme.h>
#include <hpq/Medium.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>
#include <hpq/VelledaTheme.h>
#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QApplication>
#include <QFrame>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// When Qt repaints a part of a window, the effect of the theme renders only the widgets close
// to that part. The window must still show what a repaint of the whole window shows.

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;
const QColor LineColor(0x20, 0x40, 0x80);
// A background that the themes with hachures draw in hachures, and watercolor in a wash.
const QColor PanelColor(0xf0, 0xd0, 0x60);

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed);
}

struct NamedTheme {
    std::string name;
    hpq::Theme theme;
};

// One theme for each medium, with and without hachures.
std::vector<NamedTheme> themes() {
    return {{"sketch", hpq::sketch_theme(hpq::FillStyle::Solid)},
            {"sketch with hachures", hpq::sketch_theme(hpq::FillStyle::Hachure)},
            {"slate with hachures", hpq::slate_theme(hpq::FillStyle::Hachure)},
            {"China ink", hpq::china_ink_theme(hpq::FillStyle::Solid)},
            {"Velleda with hachures", hpq::velleda_theme(hpq::FillStyle::Hachure)},
            {"watercolor", hpq::watercolor_theme(hpq::FillStyle::Solid)},
            {"watercolor with hachures", hpq::watercolor_theme(hpq::FillStyle::Hachure)}};
}

// A widget that counts its repaints, and draws a frame and a wide line along its edges.
class CountingWidget : public QWidget {
public:
    using QWidget::QWidget;

    int repaint_count() const { return m_repaint_count; }

protected:
    void paintEvent(QPaintEvent *) override {
        ++m_repaint_count;
        QPainter painter(this);
        painter.setPen(QPen(LineColor, 2.0));
        painter.drawRect(rect().adjusted(1, 1, -2, -2));
        painter.setPen(QPen(LineColor, 4.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QLineF(2, height() - 2, width() - 2, height() - 2));
    }

private:
    int m_repaint_count = 0;
};

// A widget that fills the part it is asked to paint: what it draws depends on that part.
class PartFillingWidget : public QWidget {
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(event->rect(), PanelColor);
    }
};

// What the screen shows of `window`, once the pending repaints are done.
QImage shown(QWidget &window) {
    QApplication::processEvents();
    return window.screen()->grabWindow(window.winId()).toImage().convertToFormat(QImage::Format_RGB32);
}

// The number of pixels of `window` that a repaint of the whole window changes.
int stale_pixel_count(QWidget &window) {
    const QImage before = shown(window);
    window.repaint();
    const QImage after = shown(window);
    int count = 0;
    for (int y = 0; y < after.height(); ++y) {
        for (int x = 0; x < after.width(); ++x) {
            count += before.pixel(x, y) != after.pixel(x, y) ? 1 : 0;
        }
    }
    return count;
}

// A window with widgets of every kind: views that draw what they show part by part, a
// graphics view, frames that hold other widgets, a panel that fills its background with a
// color, a scroll area and a widget of the application.
// Qt does not paint the window under a panel that fills its background with a plain color: it
// takes it as opaque. A wash of watercolor lets what is under it show, and darkens it a little
// more at each repaint: with watercolor, the window has no such panel.
class Gallery : public QWidget {
public:
    explicit Gallery(const hpq::Theme &theme) {
        resize(560, 420);
        (new QPushButton("Button", this))->setGeometry(10, 10, 100, 30);
        (new QLineEdit("Some text", this))->setGeometry(120, 10, 150, 30);
        m_counting = new CountingWidget(this);
        m_counting->setGeometry(280, 10, 120, 30);
        (new PartFillingWidget(this))->setGeometry(410, 10, 140, 30);

        auto *table = new QTableWidget(6, 3, this);
        table->setGeometry(10, 50, 260, 130);
        for (int row = 0; row < 6; ++row) {
            for (int column = 0; column < 3; ++column) {
                table->setItem(row, column, new QTableWidgetItem(QString("Cell %1").arg(row * 3 + column)));
            }
        }
        auto *tree = new QTreeWidget(this);
        tree->setGeometry(280, 50, 270, 130);
        for (int index = 0; index < 4; ++index) {
            auto *item = new QTreeWidgetItem(tree, {QString("Item %1").arg(index)});
            new QTreeWidgetItem(item, {"Child"});
            item->setExpanded(true);
        }

        auto *scene = new QGraphicsScene(this);
        scene->addEllipse(QRectF(10, 10, 80, 50), QPen(LineColor), QBrush(PanelColor));
        auto *view = new QGraphicsView(scene, this);
        view->setBackgroundBrush(PanelColor);
        view->setGeometry(10, 190, 160, 100);

        auto *box = new QGroupBox("Group", this);
        box->setGeometry(180, 190, 180, 100);
        (new QPushButton("Inside", box))->setGeometry(10, 30, 80, 30);
        (new QLabel("Label", box))->setGeometry(100, 30, 60, 30);

        auto *panel = new QFrame(this);
        panel->setGeometry(370, 190, 180, 100);
        if (theme.medium() != hpq::Medium::Watercolor) {
            QPalette panel_palette = panel->palette();
            panel_palette.setColor(QPalette::Window, PanelColor);
            panel->setPalette(panel_palette);
            panel->setAutoFillBackground(true);
        }
        (new QLabel("On a panel", panel))->setGeometry(10, 10, 120, 30);

        auto *tabs = new QTabWidget(this);
        tabs->setGeometry(10, 300, 260, 110);
        tabs->addTab(new QTextEdit("Some notes, on two lines\nof text."), "Notes");
        tabs->addTab(new QListWidget, "List");

        auto *scroll_area = new QScrollArea(this);
        scroll_area->setGeometry(280, 300, 270, 110);
        auto *content = new QWidget;
        content->resize(400, 200);
        (new QPushButton("In a scroll area", content))->setGeometry(10, 10, 150, 30);
        (new CountingWidget(content))->setGeometry(10, 50, 300, 100);
        scroll_area->setWidget(content);
    }

    CountingWidget &counting() { return *m_counting; }

private:
    CountingWidget *m_counting = nullptr;
};

}  // namespace

TEST(PartialRenderTest, ARepaintOfAButtonDoesNotPaintTheWidgetsFarFromIt) {
    const hpq::Theme theme = hpq::sketch_theme();
    Gallery window(theme);
    hpq::apply_theme(window, theme, rough_geometry_engine());
    window.show();
    shown(window);
    auto *button = window.findChild<QPushButton *>();
    ASSERT_NE(button, nullptr);

    const int repaints_before = window.counting().repaint_count();
    button->repaint();
    EXPECT_EQ(window.counting().repaint_count(), repaints_before);

    window.repaint();
    EXPECT_EQ(window.counting().repaint_count(), repaints_before + 1);
}

// A widget that draws along the edges of a repainted widget is painted again with it.
TEST(PartialRenderTest, ARepaintOfAWidgetPaintsItsCloseNeighbours) {
    QWidget window;
    window.resize(300, 100);
    auto *button = new QPushButton("Button", &window);
    button->setGeometry(10, 10, 100, 30);
    auto *neighbour = new CountingWidget(&window);
    neighbour->setGeometry(118, 10, 100, 30);
    hpq::apply_theme(window, hpq::sketch_theme(), rough_geometry_engine());
    window.show();
    shown(window);

    const int repaints_before = neighbour->repaint_count();
    button->repaint();

    EXPECT_EQ(neighbour->repaint_count(), repaints_before + 1);
}

TEST(PartialRenderTest, RepaintsOfWidgetsLeaveTheWindowAsAWholeRepaintDrawsIt) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        Gallery window(named.theme);
        hpq::apply_theme(window, named.theme, rough_geometry_engine());
        window.show();
        shown(window);

        for (QWidget *widget : window.findChildren<QWidget *>()) {
            if (widget->isVisible()) {
                widget->repaint();
            }
        }

        EXPECT_EQ(stale_pixel_count(window), 0);
    }
}

TEST(PartialRenderTest, RepaintsOfPartsLeaveTheWindowAsAWholeRepaintDrawsIt) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        Gallery window(named.theme);
        hpq::apply_theme(window, named.theme, rough_geometry_engine());
        window.show();
        shown(window);

        for (int index = 0; index < 80; ++index) {
            window.repaint(QRect((index * 97) % 560, (index * 61) % 420, 6 + (index * 13) % 50, 6 + (index * 29) % 40));
        }

        EXPECT_EQ(stale_pixel_count(window), 0);
    }
}
