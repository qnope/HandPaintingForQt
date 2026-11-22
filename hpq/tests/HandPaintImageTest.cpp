#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <functional>

#include <cstdint>

namespace {

// The base seed of the rough geometry engines: the tests draw the same paths at every run.
constexpr std::uint32_t BaseSeed = 2024;

constexpr int TargetSize = 200;
constexpr int SourceSize = 10;

// A SourceSize x SourceSize image made of 4 quarters of different colors.
QImage four_color_image() {
    QImage image(SourceSize, SourceSize, QImage::Format_ARGB32);
    const int half = SourceSize / 2;
    for (int y = 0; y < SourceSize; ++y) {
        for (int x = 0; x < SourceSize; ++x) {
            const bool right = x >= half;
            const bool bottom = y >= half;
            const QColor color = bottom ? (right ? Qt::yellow : Qt::blue) : (right ? Qt::green : Qt::red);
            image.setPixelColor(x, y, color);
        }
    }
    return image;
}

class HandPaintImageTest : public testing::Test {
protected:
    // Paints on a HandPaintDevice whose target is a white image, and returns the target image.
    QImage paint_on_device(const std::function<void(QPainter &)> &paint) {
        QImage target(TargetSize, TargetSize, QImage::Format_ARGB32);
        target.fill(Qt::white);
        QPainter target_painter(&target);
        {
            hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine);
            QPainter painter(&device);
            paint(painter);
        }
        target_painter.end();
        return target;
    }

    // Checks that the target pixels of the SourceSize x SourceSize square at `top_left` equal the source pixels.
    static void expect_source_at(const QImage &target, const QImage &source, const QPoint &top_left) {
        for (int y = 0; y < SourceSize; ++y) {
            for (int x = 0; x < SourceSize; ++x) {
                EXPECT_EQ(target.pixelColor(top_left + QPoint(x, y)), source.pixelColor(x, y))
                    << "at " << top_left.x() + x << "," << top_left.y() + y;
            }
        }
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    hpq::RoughGeometryEngine m_geometry_engine{BaseSeed};
    const QImage m_source = four_color_image();
};

TEST_F(HandPaintImageTest, DrawImageReachesTargetUnchanged) {
    const QImage target = paint_on_device([&](QPainter &painter) { painter.drawImage(QPoint(20, 20), m_source); });

    expect_source_at(target, m_source, QPoint(20, 20));
}

TEST_F(HandPaintImageTest, DrawPixmapReachesTargetUnchanged) {
    const QPixmap pixmap = QPixmap::fromImage(m_source);
    const QImage target =
        paint_on_device([&](QPainter &painter) { painter.drawPixmap(QRect(50, 50, SourceSize, SourceSize), pixmap); });

    expect_source_at(target, m_source, QPoint(50, 50));
}

TEST_F(HandPaintImageTest, DrawTiledPixmapFillsTargetAreaWithTiles) {
    const QPixmap pixmap = QPixmap::fromImage(m_source);
    const QRect area(0, 100, 40, 20);
    const QImage target = paint_on_device([&](QPainter &painter) { painter.drawTiledPixmap(area, pixmap); });

    for (int y = area.top(); y <= area.bottom(); y += SourceSize) {
        for (int x = area.left(); x <= area.right(); x += SourceSize) {
            expect_source_at(target, m_source, QPoint(x, y));
        }
    }
}

}  // namespace
