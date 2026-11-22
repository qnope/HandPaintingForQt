// Real-test probe for hand-paint-engine T10 (B17, B18): seeded randomness.
// Usage: probe <out_dir>   writes PNGs and prints comparisons.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cstdio>
#include <optional>

static void draw(QPainter& p)
{
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(20, 20, 120, 80));
    p.setBrush(Qt::blue);
    p.drawEllipse(QRectF(160, 30, 110, 70));
    p.drawPolygon(QPolygonF({QPointF(40, 140), QPointF(140, 130), QPointF(120, 220), QPointF(30, 200)}));
}

static QImage paint_on(hpq::HandPaintDevice& device, QImage& image)
{
    QPainter painter(&device);
    draw(painter);
    painter.end();
    return image.copy();
}

static QImage make(const hpq::Theme& theme, std::optional<hpq::Seed> seed)
{
    QImage image(300, 240, QImage::Format_ARGB32);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    if (seed)
        device.set_seed(*seed);
    return paint_on(device, image);
}

static long diff_pixels(const QImage& a, const QImage& b)
{
    if (a.size() != b.size()) return -1;
    long n = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y)) ++n;
    return n;
}

static void report(const char* what, const QImage& a, const QImage& b)
{
    long d = diff_pixels(a, b);
    std::printf("%-55s equal=%s differing_pixels=%ld\n", what, a == b ? "yes" : "no", d);
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QString out = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    const hpq::Theme theme = hpq::Theme::sketch();

    std::printf("DefaultSeed=%u\n", static_cast<unsigned>(hpq::HandPaintDevice::DefaultSeed));
    {
        QImage img(10, 10, QImage::Format_ARGB32);
        hpq::HandPaintDevice d(img, theme);
        std::printf("fresh device seed()=%u\n", static_cast<unsigned>(d.seed()));
        d.set_seed(42);
        std::printf("after set_seed(42) seed()=%u\n", static_cast<unsigned>(d.seed()));
    }

    QImage s42a = make(theme, 42), s42b = make(theme, 42);
    report("B17 seed 42, device A vs device B", s42a, s42b);
    QImage s1 = make(theme, 1), s2 = make(theme, 2);
    report("B18 seed 1 vs seed 2", s1, s2);
    QImage d1 = make(theme, std::nullopt), d2 = make(theme, std::nullopt);
    report("B18 no seed device A vs device B", d1, d2);
    QImage dseed = make(theme, hpq::HandPaintDevice::DefaultSeed);
    report("B18 no seed vs explicit DefaultSeed", d1, dseed);
    report("no seed vs seed 42", d1, s42a);

    // Same device painted twice.
    {
        QImage image(300, 240, QImage::Format_ARGB32);
        image.fill(Qt::red);
        hpq::HandPaintDevice device(image, theme);
        device.set_seed(7);
        QImage first = paint_on(device, image);
        QImage second = paint_on(device, image);
        report("same device (seed 7) painted twice", first, second);
        device.set_seed(8);
        QImage third = paint_on(device, image);
        report("same device after set_seed(8) vs seed 7", first, third);
    }
    // Other themes too
    for (auto [name, th] : {std::pair{"china_ink", hpq::Theme::china_ink()}, std::pair{"chalk", hpq::Theme::chalk()}}) {
        report((std::string("B17 ") + name + " seed 42 A vs B").c_str(), make(th, 42), make(th, 42));
        report((std::string("B18 ") + name + " seed 1 vs 2").c_str(), make(th, 1), make(th, 2));
    }

    s42a.save(out + "/seed42_a.png");
    s42b.save(out + "/seed42_b.png");
    s1.save(out + "/seed1.png");
    s2.save(out + "/seed2.png");
    d1.save(out + "/noseed.png");
    return 0;
}
