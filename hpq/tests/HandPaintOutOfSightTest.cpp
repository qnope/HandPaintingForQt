#include "EffectCache.h"
#include "ImageComparison.h"
#include "RecordingPaintDevice.h"

#include <hpq/ChinaInkTheme.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>
#include <hpq/VelledaTheme.h>
#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QTableWidget>
#include <QWidget>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// What would land out of what the target can paint is not drawn: not even made rough. What
// shows through the clip of the target is drawn as a drawing of the whole would draw it.

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;
const QSize SceneSize(240, 180);
// The part of the scene that the target can paint.
const QRect Part(60, 30, 90, 80);
const QColor LineColor(0x20, 0x40, 0x80);
const QColor FillColor(0xc0, 0x40, 0x40);
const QColor BandColor(0x9c, 0xcb, 0xe4);

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

struct NamedTheme {
    std::string name;
    hpq::Theme theme;
};

// One theme for each medium, with and without hachures.
std::vector<NamedTheme> themes() {
    return {{"sketch", hpq::sketch_theme(hpq::FillStyle::Solid)},
            {"sketch with hachures", hpq::sketch_theme(hpq::FillStyle::Hachure)},
            {"slate", hpq::slate_theme(hpq::FillStyle::Solid)},
            {"slate with hachures", hpq::slate_theme(hpq::FillStyle::Hachure)},
            {"China ink", hpq::china_ink_theme(hpq::FillStyle::Solid)},
            {"Velleda", hpq::velleda_theme(hpq::FillStyle::Solid)},
            {"watercolor", hpq::watercolor_theme(hpq::FillStyle::Solid)},
            {"watercolor with hachures", hpq::watercolor_theme(hpq::FillStyle::Hachure)}};
}

// Shapes, a text and an image all around Part, just out of it: their wobble, the width of
// their line, their hachures or their wash reach into it.
void draw_around_part(QPainter &painter) {
    // Left: a frame that ends a pixel before Part, and a wide line along its edge.
    painter.setPen(QPen(LineColor, 2.0));
    painter.drawRect(QRectF(10, 40, 49, 30));
    painter.setPen(QPen(LineColor, 6.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QLineF(55, 80, 55, 140));
    // Above: a filled shape that ends a pixel above Part, and a text whose letters go down into it.
    painter.setPen(QPen(LineColor, 1.0));
    painter.setBrush(FillColor);
    painter.drawRect(QRectF(70, 5, 60, 24));
    QFont font;
    font.setPixelSize(20);
    painter.setFont(font);
    painter.drawText(QPointF(140, 33), "Ag");
    // Right: a band of color, without outline, that starts a pixel after Part.
    painter.setPen(Qt::NoPen);
    painter.setBrush(BandColor);
    painter.drawRect(QRectF(151, 40, 60, 40));
    // Below: an ellipse a pixel under Part, and an image whose edge is on the edge of Part.
    painter.setPen(QPen(LineColor, 1.5));
    painter.setBrush(FillColor);
    painter.drawEllipse(QRectF(80, 111, 60, 30));
    QImage image(20, 20, QImage::Format_ARGB32);
    image.fill(BandColor);
    painter.drawImage(QPointF(140, 110), image);
}

QImage paper(const QSize &size, double device_pixel_ratio) {
    QImage image(size * device_pixel_ratio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(device_pixel_ratio);
    image.fill(Qt::white);
    return image;
}

// What `draw` draws by hand on a device of `device_pixel_ratio`, through `clip` when it is not
// empty. With a cache, what is made to draw the shapes is kept there.
QImage drawn(const hpq::Theme &theme, double device_pixel_ratio, void (*draw)(QPainter &),
             const QRect &clip = QRect(), hpq::PaintCache *cache = nullptr) {
    const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
    QImage image = paper(SceneSize, device_pixel_ratio);
    QPainter target(&image);
    if (!clip.isEmpty()) {
        target.setClipRect(clip);
    }
    hpq::HandPaintDevice device(target, theme, geometry_engine, cache);
    QPainter painter(&device);
    draw(painter);
    painter.end();
    target.end();
    return image;
}

// `image` with only `part` of it, in logical pixels, painted on white paper.
QImage only(const QImage &image, const QRect &part) {
    QImage result = paper(SceneSize, image.devicePixelRatio());
    QPainter painter(&result);
    painter.setClipRect(part);
    painter.drawImage(QPointF(), image);
    return result;
}

}  // namespace

TEST(HandPaintOutOfSightTest, WhatReachesThePaintedPartIsDrawnThere) {
    for (const NamedTheme &named : themes()) {
        for (const double ratio : {1.0, 2.0}) {
            SCOPED_TRACE(named.name + " at " + std::to_string(ratio));
            const QImage whole = drawn(named.theme, ratio, draw_around_part);
            const QImage part = drawn(named.theme, ratio, draw_around_part, Part);

            // The scene does reach into the part: the test would not tell anything otherwise.
            EXPECT_NE(only(whole, Part), paper(SceneSize, ratio));
            EXPECT_EQ(part, only(whole, Part));
        }
    }
}

TEST(HandPaintOutOfSightTest, ShapesOutOfThePaintedPartAreNotMadeRough) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        // What is made for a shape is kept in the cache: the cache tells whether anything was made.
        hpq::PaintCache cache;

        drawn(named.theme, 1.0, [](QPainter &painter) {
            painter.setPen(QPen(LineColor, 2.0));
            painter.setBrush(FillColor);
            painter.drawRect(QRectF(170, 120, 50, 40));
            painter.drawEllipse(QRectF(5, 130, 40, 30));
            painter.drawLine(QLineF(10, 5, 220, 8));
        }, Part, &cache);
        EXPECT_EQ(cache.made_count(), 0u);

        drawn(named.theme, 1.0, [](QPainter &painter) {
            painter.setPen(QPen(LineColor, 2.0));
            painter.setBrush(FillColor);
            painter.drawRect(QRectF(100, 60, 30, 30));
        }, Part, &cache);
        EXPECT_GT(cache.made_count(), 0u);
    }
}

TEST(HandPaintOutOfSightTest, TextsAndImagesOutOfThePaintedPartAreNotDrawn) {
    const hpq::Theme theme = hpq::sketch_theme();
    const hpq::RoughGeometryEngine geometry_engine(BaseSeed);
    QImage image(20, 20, QImage::Format_ARGB32);
    image.fill(BandColor);
    const auto calls_of = [&](const QPointF &place) {
        RecordingPaintDevice target;
        QPainter target_painter(&target);
        target_painter.setClipRect(QRect(60, 60, 80, 80));
        hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
        QPainter painter(&device);
        QFont font;
        font.setPixelSize(16);
        painter.setFont(font);
        painter.drawText(place, "Hello");
        painter.drawImage(place, image);
        painter.end();
        return target.engine().calls();
    };

    EXPECT_TRUE(calls_of(QPointF(150, 180)).empty());
    EXPECT_TRUE(calls_of(QPointF(5, 20)).empty());
    const std::vector<DrawCall> inside = calls_of(QPointF(80, 100));
    ASSERT_EQ(inside.size(), 2u);
    EXPECT_EQ(inside[0].kind, DrawCall::Kind::Text);
    EXPECT_EQ(inside[1].kind, DrawCall::Kind::Pixmap);
}

namespace {

// A widget that draws a new shape at each repaint: a shape that its theme has never made rough.
class ChangingShapeWidget : public QWidget {
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setPen(QPen(LineColor, 2.0));
        painter.drawRect(QRectF(10 + 0.25 * m_repaint_count, 10, 50, 30));
        ++m_repaint_count;
    }

private:
    int m_repaint_count = 0;
};

// What the screen shows of `window`, once the pending repaints are done.
QImage shown(QWidget &window) {
    QApplication::processEvents();
    return window.screen()->grabWindow(window.winId()).toImage().convertToFormat(QImage::Format_RGB32);
}

}  // namespace

// When Qt repaints one widget, the effect of the theme still renders the whole window: the
// shapes far from that widget are not drawn, so they are not made rough.
TEST(HandPaintOutOfSightTest, ARepaintOfAWidgetDoesNotMakeTheShapesOfAnotherRough) {
    QWidget window;
    window.resize(400, 200);
    auto *button = new QPushButton("Button", &window);
    button->setGeometry(10, 10, 100, 30);
    auto *far_widget = new ChangingShapeWidget(&window);
    far_widget->setGeometry(250, 100, 120, 80);
    hpq::apply_theme(window, hpq::sketch_theme(), std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed));
    window.show();
    shown(window);
    // The button may have taken the focus since the window was shown: its frame is drawn now.
    button->repaint();

    const std::size_t made_before = hpq::test::made_count_of(window);
    button->repaint();
    EXPECT_EQ(hpq::test::made_count_of(window), made_before);

    far_widget->repaint();
    EXPECT_GT(hpq::test::made_count_of(window), made_before);
}

// After repaints of small parts here and there, the window shows what a repaint of the whole
// window shows.
TEST(HandPaintOutOfSightTest, RepaintsOfPartsLeaveTheWindowAsAWholeRepaintDrawsIt) {
    for (const NamedTheme &named : themes()) {
        SCOPED_TRACE(named.name);
        QWidget window;
        window.resize(360, 240);
        (new QPushButton("Button", &window))->setGeometry(10, 10, 100, 30);
        (new QLabel("A label", &window))->setGeometry(130, 10, 100, 30);
        (new QLineEdit("Some text", &window))->setGeometry(10, 50, 220, 30);
        auto *table = new QTableWidget(4, 3, &window);
        table->setGeometry(10, 90, 340, 140);
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 3; ++column) {
                table->setItem(row, column, new QTableWidgetItem(QString("Cell %1").arg(row * 3 + column)));
            }
        }
        hpq::apply_theme(window, named.theme, std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed));
        window.show();
        shown(window);

        for (int index = 0; index < 60; ++index) {
            window.repaint(QRect((index * 97) % 360, (index * 61) % 240, 10 + (index * 13) % 60, 8 + (index * 29) % 40));
        }
        const QImage after_parts = shown(window);
        window.repaint();

        EXPECT_EQ(hpq::test::differing_pixel_count(after_parts, shown(window)), 0);
    }
}
