// A transform left set at the end of the first paint must not leak into the next paint on the same device.
// Usage: repaint3 <out_dir>
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cstdio>
#include <functional>

static void draw(QPainter& p)
{
    p.drawRect(QRectF(20, 20, 120, 80));
    p.setBrush(Qt::blue);
    p.drawEllipse(QRectF(160, 30, 110, 70));
    p.drawPolygon(QPolygonF({QPointF(40, 140), QPointF(140, 130), QPointF(120, 220), QPointF(30, 200)}));
}

static QImage paint(hpq::HandPaintDevice& device, QImage& image, const std::function<void(QPainter&)>& f)
{
    { QPainter p(&device); f(p); }
    return image.copy();
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const QString out = QString::fromLocal8Bit(argv[1]);
    const hpq::Theme theme = hpq::Theme::sketch();
    for (bool seeded : {true, false}) {
        QImage ref_img(300, 240, QImage::Format_ARGB32);
        hpq::HandPaintDevice ref_dev(ref_img, theme);
        if (seeded) ref_dev.set_seed(7);
        const QImage ref = paint(ref_dev, ref_img, draw);

        QImage image(300, 240, QImage::Format_ARGB32);
        image.fill(Qt::red);
        hpq::HandPaintDevice device(image, theme);
        if (seeded) device.set_seed(7);
        // First paint ends with a translate + scale + rotate, a pen off and a brush on, all left set.
        const QImage first = paint(device, image, [](QPainter& p) {
            draw(p);
            p.translate(60, 40);
            p.scale(0.5, 0.5);
            p.rotate(20);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 255, 0, 128)); // half-transparent brush left set
            p.drawRect(QRectF(0, 0, 10, 10));
        });
        const QImage second = paint(device, image, draw);
        const char* tag = seeded ? "seed7" : "noseed";
        std::printf("%s: transform/pen/brush left set at end of paint 1; paint 2 == fresh device: %d\n", tag,
                    second == ref);
        second.save(out + "/transform_left_set_second_" + tag + ".png");
        ref.save(out + "/transform_left_set_fresh_" + tag + ".png");
    }
    return 0;
}
