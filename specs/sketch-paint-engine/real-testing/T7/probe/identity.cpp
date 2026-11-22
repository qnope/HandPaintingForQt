// Identity transform: text and pixmap drawn on the device land where they are drawn directly.
#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <cstdio>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QPixmap pm(100, 60);
    pm.fill(QColor(30, 160, 60));
    { QPainter pp(&pm); pp.setPen(QPen(Qt::yellow, 4)); pp.drawLine(0, 0, 100, 60); pp.drawLine(0, 60, 100, 0); }
    QFont f("Helvetica"); f.setPixelSize(20);
    QImage imgs[2];
    for (int h = 0; h < 2; ++h) {
        QImage img(400, 200, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter target(&img);
        auto draw = [&](QPainter &p) {
            p.drawPixmap(QPointF(250, 100), pm);
            p.setPen(Qt::darkMagenta); p.setFont(f);
            p.drawText(QPointF(30, 60), "Identity text");
            p.setBrush(QColor(255, 200, 120)); p.setPen(QPen(Qt::black, 2));
            p.drawRect(QRectF(30, 100, 120, 70));
        };
        if (h) { hpq::SketchTheme t; hpq::RoughGeometryEngine ge; hpq::HandPaintDevice dev(target, t, ge); QPainter p(&dev); draw(p); p.end(); }
        else draw(target);
        target.end();
        img.save(h ? "identity_hand.png" : "identity_reference.png");
        imgs[h] = img;
    }
    std::printf("pixmap region identical hand vs reference: %s\n", imgs[0].copy(250, 100, 100, 60) == imgs[1].copy(250, 100, 100, 60) ? "yes" : "no");
    for (int h = 0; h < 2; ++h) {
        int n = 0; QRect r;
        for (int y = 0; y < 95; ++y) for (int x = 0; x < 400; ++x) { QColor c = imgs[h].pixelColor(x, y); if (c != Qt::white) { ++n; r |= QRect(x, y, 1, 1); } }
        std::printf("%s text ink: %d px, bbox (%d,%d %dx%d)\n", h ? "hand" : "reference", n, r.x(), r.y(), r.width(), r.height());
    }
    return 0;
}
