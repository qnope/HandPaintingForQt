#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <cstdio>

static QImage icon(int n) {
    QImage img(n, n, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            QColor c = (x < n/2) ? (y < n/2 ? Qt::red : Qt::blue) : (y < n/2 ? Qt::green : QColor(255,200,0));
            if (n > 10 && (x - n/2)*(x - n/2) + (y - n/2)*(y - n/2) < (n/5)*(n/5)) c = Qt::black;
            img.setPixelColor(x, y, c);
        }
    return img;
}

template <class F> static QImage render(bool through, F f) {
    QImage target(200, 200, QImage::Format_ARGB32_Premultiplied);
    target.fill(Qt::white);
    QPainter tp(&target);
    hpq::SketchTheme theme; hpq::RoughGeometryEngine geo;
    if (through) {
        hpq::HandPaintDevice dev(tp, theme, geo);
        QPainter p(&dev);
        f(p);
        p.end();
    } else {
        f(tp);
    }
    tp.end();
    return target;
}

static int diff(const QImage &a, const QImage &b, const QRect &r) {
    int n = 0;
    for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x)
        if (a.pixel(x, y) != b.pixel(x, y)) ++n;
    (void)r; return n;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";
    QImage img10 = icon(10); QPixmap pix10 = QPixmap::fromImage(img10);
    QImage img32 = icon(32); QPixmap pix32 = QPixmap::fromImage(img32);
    img10.scaled(100,100).save(out + "/source10_x10.png");
    img32.scaled(128,128).save(out + "/source32_x4.png");
    int failures = 0;
    struct Case { const char *name; std::function<void(QPainter&)> f; QRect area; };
    std::vector<Case> cases = {
        {"drawImage_point", [&](QPainter &p){ p.drawImage(QPoint(20,20), img10); }, QRect(20,20,10,10)},
        {"drawPixmap_rect", [&](QPainter &p){ p.drawPixmap(QRect(50,50,10,10), pix10); }, QRect(50,50,10,10)},
        {"drawTiledPixmap", [&](QPainter &p){ p.drawTiledPixmap(QRect(0,100,40,20), pix10); }, QRect(0,100,40,20)},
        {"icon32_all", [&](QPainter &p){ p.drawImage(QPoint(120,10), img32); p.drawPixmap(QRect(120,60,64,64), pix32);
            p.drawPixmap(QPointF(10,150), pix32, QRectF(16,0,16,32)); p.drawTiledPixmap(QRect(60,140,130,50), pix32, QPointF(5,5)); }, QRect()},
        {"mixed_with_rect", [&](QPainter &p){ p.setPen(Qt::black); p.setBrush(Qt::NoBrush); p.drawImage(QPoint(20,20), img10); p.setOpacity(1.0); }, QRect()},
    };
    for (auto &c : cases) {
        QImage a = render(true, c.f), b = render(false, c.f);
        a.save(out + "/" + c.name + "_device.png");
        b.save(out + "/" + c.name + "_direct.png");
        a.scaled(600,600).save(out + "/" + c.name + "_device_x3.png");
        int d = diff(a, b, c.area);
        bool exact = true;
        if (c.area.isValid()) {
            for (int y = c.area.top(); y <= c.area.bottom(); ++y)
                for (int x = c.area.left(); x <= c.area.right(); ++x) {
                    int sx = (x - c.area.left()) % 10, sy = (y - c.area.top()) % 10;
                    if (a.pixel(x, y) != img10.pixel(sx, sy)) exact = false;
                }
        }
        std::printf("%-16s differing pixels device vs direct: %d ; area equals source tiles: %s\n", c.name, d,
                    c.area.isValid() ? (exact ? "yes" : "NO") : "n/a");
        if (d || !exact) ++failures;
    }
    // target state preserved after image calls: draw image via device, then a direct fillRect on target with its own state
    {
        QImage target(200,200,QImage::Format_ARGB32_Premultiplied); target.fill(Qt::white);
        QPainter tp(&target); tp.setBrush(Qt::magenta); tp.setPen(Qt::cyan); tp.setOpacity(0.5);
        hpq::SketchTheme theme; hpq::RoughGeometryEngine geo;
        { hpq::HandPaintDevice dev(tp, theme, geo); QPainter p(&dev); p.drawImage(QPoint(0,0), img10); p.drawPixmap(0,0,pix10); p.end(); }
        std::printf("target state after: brush=%s pen=%s opacity=%.2f\n", qPrintable(tp.brush().color().name()), qPrintable(tp.pen().color().name()), tp.opacity());
        tp.end();
    }
    std::printf("failures: %d\n", failures);
    return failures ? 1 : 0;
}
