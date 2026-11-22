#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPicture>
#include <cstdio>

enum Mode { Both, NoPen, NoBrush };

static void count(const QImage &img, const char *name) {
    int red = 0, blue = 0, other = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x) {
            QColor c = img.pixelColor(x, y);
            if (c == Qt::white) continue;
            if (c.red() > 150 && c.blue() < 120) ++red;
            else if (c.blue() > 150 && c.red() < 120) ++blue;
            else ++other;
        }
    std::printf("%-12s red-ish px=%d blue-ish px=%d other non-white px=%d\n", name, red, blue, other);
}

static QImage draw(Mode mode, int scale) {
    QImage img(240 * scale / 2, 140 * scale / 2, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    target.scale(scale / 2.0 * 2, scale / 2.0 * 2);
    hpq::SketchTheme theme;
    hpq::RoughGeometryEngine geo;
    hpq::HandPaintDevice device(target, theme, geo);
    QPainter p(&device);
    if (!p.isActive()) std::printf("QPainter on device NOT active!\n");
    p.setPen(mode == NoPen ? QPen(Qt::NoPen) : QPen(Qt::red, 2));
    p.setBrush(mode == NoBrush ? QBrush(Qt::NoBrush) : QBrush(Qt::blue));
    p.drawRect(QRectF(10, 10, 100, 50));
    p.end();
    target.end();
    return img;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    const char *names[] = {"both.png", "nopen.png", "nobrush.png"};
    for (int m = 0; m < 3; ++m) {
        QImage img = draw(Mode(m), 2);
        img.save(names[m]);
        count(img, names[m]);
    }
    // reference: exact rect drawn directly
    QImage ref(240, 140, QImage::Format_ARGB32);
    ref.fill(Qt::white);
    { QPainter p(&ref); p.setRenderHint(QPainter::Antialiasing); p.scale(2, 2);
      p.setPen(QPen(Qt::red, 2)); p.setBrush(Qt::blue); p.drawRect(QRectF(10, 10, 100, 50)); }
    ref.save("reference_exact.png");
    count(ref, "reference");
    // determinism check
    std::printf("both drawn twice identical: %s\n", draw(Both, 2) == draw(Both, 2) ? "yes" : "no");
    // record ops via QPicture target
    QPicture pic;
    { QPainter t(&pic); hpq::SketchTheme th; hpq::RoughGeometryEngine g; hpq::HandPaintDevice d(t, th, g);
      QPainter p(&d); p.setPen(QPen(Qt::red, 2)); p.setBrush(Qt::blue); p.drawRect(QRectF(10, 10, 100, 50)); }
    std::printf("QPicture target size bytes=%u\n", pic.size());
    return 0;
}
