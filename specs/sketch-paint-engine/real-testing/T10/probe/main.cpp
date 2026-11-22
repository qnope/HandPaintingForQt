#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFontInfo>
#include <cstdio>

struct Case { const char* name; QFont font; QColor color; QString text; };

static QImage render_direct(const Case& c, bool theme_font) {
    QImage img(420, 80, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    QFont f = c.font;
    if (theme_font) { hpq::SketchTheme t; QFont tf = t.font(); tf.setPixelSize(QFontInfo(c.font).pixelSize()); f = tf; }
    p.setFont(f); p.setPen(c.color);
    p.drawText(10, 50, c.text);
    return img;
}

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    hpq::SketchTheme theme;
    hpq::RoughGeometryEngine geo;
    QFont helv("Helvetica"); helv.setPixelSize(16);
    QFont pt12("Helvetica"); pt12.setPointSize(12);
    QFont bold("Courier New"); bold.setPixelSize(28); bold.setBold(true);
    QFont big("Times New Roman"); big.setPixelSize(40);
    Case cases[] = {
        {"helvetica16_darkred", helv, QColor(Qt::darkRed), "Hello, hand-painted world!"},
        {"point12_blue", pt12, QColor(Qt::blue), "Point size 12 text 0123456789"},
        {"courier28_bold_green", bold, QColor(0,128,0), "Bold Courier 28px"},
        {"times40_black", big, QColor(Qt::black), "Times 40px gjqy"},
    };
    for (auto& c : cases) {
        QImage img(420, 80, QImage::Format_ARGB32);
        img.fill(Qt::white);
        {
            QPainter target(&img);
            hpq::HandPaintDevice dev(target, theme, geo);
            QPainter p(&dev);
            p.setFont(c.font); p.setPen(c.color);
            p.drawText(10, 50, c.text);
        }
        img.save(QString("../%1_device.png").arg(c.name));
        render_direct(c, false).save(QString("../%1_direct_original.png").arg(c.name));
        QImage ref = render_direct(c, true);
        ref.save(QString("../%1_direct_themefont.png").arg(c.name));
        // stats
        int ink = 0, colormatch = 0; QRect bb;
        for (int y = 0; y < img.height(); ++y) for (int x = 0; x < img.width(); ++x) {
            QColor px = img.pixelColor(x, y);
            if (px != QColor(Qt::white)) { ++ink; bb |= QRect(x, y, 1, 1); }
        }
        // darkest pixel color
        int best = 999; QColor darkest;
        for (int y = 0; y < img.height(); ++y) for (int x = 0; x < img.width(); ++x) {
            QColor px = img.pixelColor(x, y); int l = px.lightness(); if (l < best) { best = l; darkest = px; }
        }
        int diff = 0;
        for (int y = 0; y < img.height(); ++y) for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != ref.pixel(x, y)) ++diff;
        std::printf("%s: font px=%d ink=%d bbox=(%d,%d %dx%d) darkest=%s expected=%s diff_vs_themefont_ref=%d\n",
            c.name, QFontInfo(c.font).pixelSize(), ink, bb.x(), bb.y(), bb.width(), bb.height(),
            qPrintable(darkest.name()), qPrintable(c.color.name()), diff);
    }
    // Image + text together (after merge with T11)
    {
        QImage src(120, 60, QImage::Format_ARGB32);
        for (int y = 0; y < 60; ++y) for (int x = 0; x < 120; ++x)
            src.setPixelColor(x, y, QColor(x * 2, y * 4, 200));
        QImage img(420, 120, QImage::Format_ARGB32);
        img.fill(Qt::white);
        {
            QPainter target(&img);
            hpq::HandPaintDevice dev(target, theme, geo);
            QPainter p(&dev);
            p.drawImage(QPoint(20, 10), src);
            p.setFont(helv); p.setPen(QColor(Qt::darkRed));
            p.drawText(160, 50, "Image and text together");
            p.drawImage(QPoint(280, 60), src);
            p.drawText(20, 100, "Text after second image");
        }
        img.save("../image_and_text_device.png");
        int mism = 0;
        for (int y = 0; y < 60; ++y) for (int x = 0; x < 120; ++x) {
            if (img.pixel(20 + x, 10 + y) != src.pixel(x, y)) ++mism;
            if (img.pixel(280 + x, 60 + y) != src.pixel(x, y)) ++mism;
        }
        int textink = 0; QColor darkest; int best = 999;
        for (int y = 0; y < 120; ++y) for (int x = 150; x < 278; ++x) {
            QColor px = img.pixelColor(x, y);
            if (px != QColor(Qt::white)) { ++textink; if (px.lightness() < best) { best = px.lightness(); darkest = px; } }
        }
        std::printf("image_and_text: image_pixel_mismatches=%d text_ink_between_images=%d darkest_text=%s\n",
            mism, textink, qPrintable(darkest.name()));
    }
    std::printf("theme family: %s\n", qPrintable(theme.font().family()));
    return 0;
}
