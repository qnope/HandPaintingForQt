// Repaint the same device twice and save both images; also a fresh device with seed 7.
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
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(20, 20, 120, 80));
    p.setBrush(Qt::blue);
    p.drawEllipse(QRectF(160, 30, 110, 70));
    p.drawPolygon(QPolygonF({QPointF(40, 140), QPointF(140, 130), QPointF(120, 220), QPointF(30, 200)}));
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QString out = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    const hpq::Theme theme = hpq::Theme::sketch();

    QImage fresh(300, 240, QImage::Format_ARGB32);
    {
        hpq::HandPaintDevice device(fresh, theme);
        device.set_seed(7);
        QPainter p(&device);
        draw(p);
    }

    QImage image(300, 240, QImage::Format_ARGB32);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    device.set_seed(7);
    QImage first, second;
    {
        QPainter p(&device);
        std::printf("first begin active=%d\n", p.isActive());
        draw(p);
    }
    first = image.copy();
    {
        QPainter p(&device);
        std::printf("second begin active=%d\n", p.isActive());
        draw(p);
    }
    second = image.copy();
    std::printf("fresh==first %d, first==second %d, fresh==second %d\n", fresh == first, first == second, fresh == second);
    first.save(out + "/repaint_first.png");
    second.save(out + "/repaint_second.png");
    fresh.save(out + "/repaint_fresh_seed7.png");
    return 0;
}
