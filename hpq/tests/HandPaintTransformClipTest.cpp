#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <cmath>
#include <functional>

namespace {

constexpr int TargetSize = 300;

bool is_dark(const QImage &image, int x, int y) { return qGray(image.pixel(x, y)) < 128; }

// Smallest rectangle holding every dark pixel of `image`; empty when there is none.
QRect dark_bounds(const QImage &image) {
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (is_dark(image, x, y)) {
                bounds |= QRect(x, y, 1, 1);
            }
        }
    }
    return bounds;
}

QImage black_image(int size) {
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::black);
    return image;
}

QFont text_font() {
    QFont font;
    font.setPixelSize(20);
    return font;
}

const QString Text("MWMW");

class HandPaintTransformClipTest : public testing::Test {
protected:
    // Paints with `paint` on a HandPaintDevice whose target is a white image, and returns the image.
    QImage paint_on_white_image(const std::function<void(QPainter &)> &paint) const {
        QImage image(TargetSize, TargetSize, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter target_painter(&image);
        hpq::HandPaintDevice device(target_painter, m_theme, m_geometry_engine);
        QPainter painter(&device);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        paint(painter);
        painter.end();
        target_painter.end();
        return image;
    }

    hpq::Theme m_theme = hpq::sketch_theme();
    hpq::RoughGeometryEngine m_geometry_engine;
};

TEST_F(HandPaintTransformClipTest, RoughShapeLandsAtTransformedPosition) {
    const QImage image = paint_on_white_image([](QPainter &painter) {
        painter.translate(100, 100);
        painter.scale(2, 2);
        painter.drawRect(QRectF(0, 0, 20, 20));
    });

    EXPECT_TRUE(is_dark(image, 120, 120));
    EXPECT_FALSE(is_dark(image, 20, 20));

    const int margin = static_cast<int>(std::ceil(m_theme.rough_options().max_offset * 2));
    const QRect allowed = QRect(100, 100, 40, 40).adjusted(-margin, -margin, margin, margin);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (is_dark(image, x, y)) {
                ASSERT_TRUE(allowed.contains(x, y)) << "dark pixel at " << x << "," << y;
            }
        }
    }
}

TEST_F(HandPaintTransformClipTest, NothingIsDrawnOutsideTheClip) {
    const QRect clip(0, 0, 50, 50);
    const QImage image = paint_on_white_image([&clip](QPainter &painter) {
        painter.setClipRect(clip);
        painter.drawRect(QRectF(0, 0, 200, 200));
    });

    EXPECT_TRUE(is_dark(image, 25, 25));
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!clip.contains(x, y)) {
                ASSERT_EQ(image.pixel(x, y), QColor(Qt::white).rgb()) << "pixel at " << x << "," << y;
            }
        }
    }
}

TEST_F(HandPaintTransformClipTest, TranslatedImageLandsAtTranslatedPlace) {
    const QImage image = paint_on_white_image([](QPainter &painter) {
        painter.translate(100, 100);
        painter.drawImage(QPointF(0, 0), black_image(20));
    });

    EXPECT_EQ(dark_bounds(image), QRect(100, 100, 20, 20));
}

TEST_F(HandPaintTransformClipTest, TranslatedPixmapLandsAtTranslatedPlace) {
    const QImage image = paint_on_white_image([](QPainter &painter) {
        painter.translate(100, 100);
        painter.drawPixmap(QPointF(0, 0), QPixmap::fromImage(black_image(20)));
    });

    EXPECT_EQ(dark_bounds(image), QRect(100, 100, 20, 20));
}

TEST_F(HandPaintTransformClipTest, TranslatedTiledPixmapLandsAtTranslatedPlace) {
    const QImage image = paint_on_white_image([](QPainter &painter) {
        painter.translate(100, 100);
        painter.drawTiledPixmap(QRectF(0, 0, 40, 40), QPixmap::fromImage(black_image(10)));
    });

    EXPECT_EQ(dark_bounds(image), QRect(100, 100, 40, 40));
}

TEST_F(HandPaintTransformClipTest, ClipCutsAnImage) {
    const QRect clip(0, 0, 50, 50);
    const QImage image = paint_on_white_image([&clip](QPainter &painter) {
        painter.setClipRect(clip);
        painter.drawImage(QPointF(0, 0), black_image(200));
    });

    EXPECT_EQ(dark_bounds(image), clip);
}

TEST_F(HandPaintTransformClipTest, TranslatedTextLandsAtTranslatedPlace) {
    const auto draw_text = [](QPainter &painter) {
        painter.setPen(Qt::black);
        painter.setFont(text_font());
        painter.drawText(QPointF(0, 30), Text);
    };
    const QRect untranslated = dark_bounds(paint_on_white_image(draw_text));
    const QRect translated = dark_bounds(paint_on_white_image([&draw_text](QPainter &painter) {
        painter.translate(150, 170);
        draw_text(painter);
    }));

    ASSERT_FALSE(untranslated.isEmpty());
    EXPECT_EQ(translated, untranslated.translated(150, 170));
}

TEST_F(HandPaintTransformClipTest, ScaledTextIsScaledLikeQtDoes) {
    const auto draw_text = [](QPainter &painter) {
        painter.setPen(Qt::black);
        painter.setFont(text_font());
        painter.drawText(QPointF(0, 30), Text);
    };
    const QRect unscaled = dark_bounds(paint_on_white_image(draw_text));
    const QRect scaled = dark_bounds(paint_on_white_image([&draw_text](QPainter &painter) {
        painter.scale(2, 2);
        draw_text(painter);
    }));

    ASSERT_FALSE(unscaled.isEmpty());
    EXPECT_NEAR(scaled.height(), 2 * unscaled.height(), 3);
    EXPECT_NEAR(scaled.width(), 2 * unscaled.width(), 6);
}

TEST_F(HandPaintTransformClipTest, ClipCutsText) {
    const QRect clip(0, 0, 300, 25);
    const QImage image = paint_on_white_image([&clip](QPainter &painter) {
        painter.setClipRect(clip);
        painter.setPen(Qt::black);
        painter.setFont(text_font());
        painter.drawText(QPointF(0, 30), Text);
    });

    const QRect bounds = dark_bounds(image);
    ASSERT_FALSE(bounds.isEmpty());
    EXPECT_TRUE(clip.contains(bounds));
}

}  // namespace
