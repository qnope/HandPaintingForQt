#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cstdio>

struct Case { const char* name; hpq::Theme theme; };

static int check(const char* label, const QImage& image, const QColor& bg)
{
    int bad = 0;
    QRgb first_bad = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x) {
            QColor c = image.pixelColor(x, y);
            if (c.rgba() != bg.rgba()) { if (!bad) first_bad = c.rgba(); ++bad; }
        }
    std::printf("%s: %dx%d expected %s -> %d non-background pixels%s",
                label, image.width(), image.height(), qPrintable(bg.name(QColor::HexArgb)), bad,
                bad ? "" : "\n");
    if (bad) std::printf(" (first bad #%08x)\n", first_bad);
    return bad ? 1 : 0;
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const char* out = argc > 1 ? argv[1] : ".";
    Case cases[] = {{"sketch", hpq::Theme::sketch()},
                    {"china_ink", hpq::Theme::china_ink()},
                    {"chalk", hpq::Theme::chalk()}};
    int failures = 0;
    for (auto& c : cases) {
        QImage image(200, 200, QImage::Format_ARGB32);
        image.fill(Qt::red);
        image.save(QString("%1/%2_before.png").arg(out, c.name));
        {
            hpq::HandPaintDevice device(image, c.theme);
            std::printf("device %dx%d\n", device.width(), device.height());
            QPainter painter;
            bool began = painter.begin(&device);
            std::printf("%s: begin=%d active=%d\n", c.name, began, painter.isActive());
            bool ended = painter.end();
            std::printf("%s: end=%d active-after=%d\n", c.name, ended, painter.isActive());
            failures += !began || !ended;
        }
        image.save(QString("%1/%2_after.png").arg(out, c.name));
        failures += check(c.name, image, c.theme.background());

        // Same with the painter constructor form and a second cycle on the same device.
        QImage image2(200, 200, QImage::Format_RGB32);
        image2.fill(Qt::red);
        hpq::HandPaintDevice device2(image2, c.theme);
        { QPainter p(&device2); std::printf("%s ctor-form active=%d\n", c.name, p.isActive()); }
        image2.fill(Qt::red);
        { QPainter p(&device2); std::printf("%s second-cycle active=%d\n", c.name, p.isActive()); }
        failures += check((std::string(c.name) + " RGB32 2nd cycle").c_str(), image2, c.theme.background());
    }
    std::printf("failures=%d\n", failures);
    return failures;
}
