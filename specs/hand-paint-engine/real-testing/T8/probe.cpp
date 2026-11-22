// Real-test probe for T8 (B14, B15): uses hpq like a user would.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <cstdio>
#include <string>

namespace {

QImage blank() { return QImage(200, 200, QImage::Format_ARGB32_Premultiplied); }

QFont fixed_font()
{
    QFont font("Helvetica");
    font.setPixelSize(24);
    return font;
}

int diff_count(const QImage& a, const QImage& b)
{
    if (a.size() != b.size()) return -1;
    int n = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y)) ++n;
    return n;
}

int count_color(const QImage& img, QRgb c)
{
    int n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) == c) ++n;
    return n;
}

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";
    int failures = 0;

    struct Named { const char* name; hpq::Theme theme; };
    const Named themes[] = {{"sketch", hpq::Theme::sketch()},
                            {"china_ink", hpq::Theme::china_ink()},
                            {"chalk", hpq::Theme::chalk()}};

    QImage red_source(40, 40, QImage::Format_ARGB32_Premultiplied);
    red_source.fill(QColor(255, 0, 0));
    const QPixmap red_pixmap = QPixmap::fromImage(red_source);

    for (const auto& [name, theme] : themes) {
        // ---- B14: text
        QImage hand = blank();
        {
            hpq::HandPaintDevice device(hand, theme);
            QPainter p(&device);
            p.setFont(fixed_font());
            p.setPen(QPen(Qt::red));
            p.drawText(QPointF(20, 100), "Hello");
        }
        QImage plain = blank();
        {
            plain.fill(theme.background());
            QPainter p(&plain);
            p.setFont(fixed_font());
            p.setPen(QPen(theme.ink()));
            p.drawText(QPointF(20, 100), "Hello");
        }
        // Also the "user's naive expectation" baseline: red text drawn with plain QPainter.
        QImage plain_red = blank();
        {
            plain_red.fill(theme.background());
            QPainter p(&plain_red);
            p.setFont(fixed_font());
            p.setPen(QPen(Qt::red));
            p.drawText(QPointF(20, 100), "Hello");
        }
        hand.save(QString::fromStdString(out + "/b14_" + name + "_hand.png"));
        plain.save(QString::fromStdString(out + "/b14_" + name + "_plain.png"));
        const int d = diff_count(hand, plain);
        const int red_px = count_color(hand, qRgb(255, 0, 0));
        const int ink_px = count_color(hand, theme.ink().rgb());
        const int bg_px = count_color(hand, theme.background().rgb());
        std::printf("B14 %-9s text: diff vs plain(ink)=%d, diff vs plain(red)=%d, pure-red px=%d, ink px=%d, bg px=%d -> %s\n",
                    name, d, diff_count(hand, plain_red), red_px, ink_px, bg_px, d == 0 ? "OK" : "MISMATCH");
        if (d != 0) ++failures;

        // ---- B15: drawImage and drawPixmap
        for (int mode = 0; mode < 2; ++mode) {
            QImage img = blank();
            {
                hpq::HandPaintDevice device(img, theme);
                QPainter p(&device);
                p.setPen(QPen(Qt::red));
                if (mode == 0)
                    p.drawImage(QPointF(80, 80), red_source);
                else
                    p.drawPixmap(QPointF(80, 80), red_pixmap);
            }
            const char* kind = mode == 0 ? "image" : "pixmap";
            img.save(QString::fromStdString(out + "/b15_" + name + "_" + kind + ".png"));
            const QImage region = img.copy(80, 80, 40, 40).convertToFormat(red_source.format());
            const int rd = diff_count(region, red_source);
            // Outside the region: background only?
            int outside_non_bg = 0;
            for (int y = 0; y < img.height(); ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (!(x >= 80 && x < 120 && y >= 80 && y < 120) && img.pixel(x, y) != theme.background().rgb())
                        ++outside_non_bg;
            std::printf("B15 %-9s draw%-6s: region diff vs source=%d, non-background px outside=%d -> %s\n", name,
                        kind, rd, outside_non_bg, rd == 0 ? "OK" : "MISMATCH");
            if (rd != 0) ++failures;
        }
    }
    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
