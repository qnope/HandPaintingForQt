// Same as Repaint, but the drawing forces a brush change before NoBrush, so the engine gets the brush state.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cstdio>
static void draw(QPainter& p)
{
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::green);
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(20, 20, 120, 80));
    p.setBrush(Qt::blue);
    p.drawEllipse(QRectF(160, 30, 110, 70));
    p.drawPolygon(QPolygonF({QPointF(40, 140), QPointF(140, 130), QPointF(120, 220), QPointF(30, 200)}));
}
int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QImage image(300, 240, QImage::Format_ARGB32);
    hpq::HandPaintDevice device(image, hpq::Theme::sketch());
    device.set_seed(7);
    { QPainter p(&device); draw(p); }
    QImage first = image.copy();
    { QPainter p(&device); draw(p); }
    std::printf("explicit brush reset: first==second %d\n", first == image);
    // Unseeded device repainted twice with the original drawing (no brush on the first painter state).
    QImage image2(300, 240, QImage::Format_ARGB32);
    hpq::HandPaintDevice device2(image2, hpq::Theme::sketch());
    auto plain = [](QPainter& p) { p.drawRect(QRectF(20, 20, 120, 80)); p.setBrush(Qt::blue); p.drawEllipse(QRectF(160, 30, 110, 70)); };
    { QPainter p(&device2); plain(p); }
    QImage a = image2.copy();
    { QPainter p(&device2); plain(p); }
    std::printf("default seed device, repaint (rect default brush, then blue ellipse): first==second %d\n", a == image2);
    image2.save(QString::fromLocal8Bit(argv[1]) + "/default_seed_second_paint.png");
    return 0;
}
