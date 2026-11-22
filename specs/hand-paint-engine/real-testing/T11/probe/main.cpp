// Real-test probe for B19: same drawing with Sketch, China Ink and Chalk on hpq::HandPaintDevice.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

QImage draw_scene(const hpq::Theme& theme)
{
    QImage image(420, 300, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::magenta); // anything not covered by the theme background would show up
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::red, 3));
    painter.setBrush(Qt::green);
    painter.drawRect(QRectF(30, 30, 140, 100));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(220, 30, 160, 100));
    painter.setBrush(Qt::blue);
    painter.drawPolygon(QPolygonF({QPointF(60, 170), QPointF(180, 260), QPointF(30, 270)}));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(QPolygonF({QPointF(240, 170), QPointF(380, 180), QPointF(330, 270), QPointF(250, 250)}));
    QFont font = painter.font();
    font.setPointSize(16);
    painter.setFont(font);
    painter.drawText(QPointF(215, 160), "Hello hpq");
    painter.end();
    return image;
}

// Is pixel = a*bg + b*ink + c*fill (a,b,c >= 0, sum 1) within tol per channel? Brute-force grid search.
bool is_blend(QRgb p, const hpq::Theme& t, double tol)
{
    if (qAlpha(p) != 255)
        return false;
    const QColor bg = t.background(), ink = t.ink(), fill = t.fill();
    const int steps = 255;
    for (int i = 0; i <= steps; ++i)
    {
        const double b = double(i) / steps;
        for (int j = 0; i + j <= steps; ++j)
        {
            const double c = double(j) / steps;
            const double a = 1.0 - b - c;
            const double r = a * bg.red() + b * ink.red() + c * fill.red();
            const double g = a * bg.green() + b * ink.green() + c * fill.green();
            const double bl = a * bg.blue() + b * ink.blue() + c * fill.blue();
            if (std::abs(r - qRed(p)) <= tol && std::abs(g - qGreen(p)) <= tol && std::abs(bl - qBlue(p)) <= tol)
                return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";
    struct Named { const char* name; hpq::Theme theme; };
    std::vector<Named> themes = {{"sketch", hpq::Theme::sketch()}, {"china_ink", hpq::Theme::china_ink()},
                                 {"chalk", hpq::Theme::chalk()}};
    std::vector<QImage> images;
    int failures = 0;
    for (const auto& n : themes)
    {
        QImage img = draw_scene(n.theme).convertToFormat(QImage::Format_ARGB32);
        img.save(QString::fromStdString(out + "/" + n.name + ".png"));
        // Cache checked colors.
        std::vector<QRgb> seen;
        int bad = 0, bad_unique = 0, distinct = 0;
        QRgb first_bad = 0;
        std::vector<std::pair<QRgb, bool>> cache;
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x)
            {
                const QRgb p = img.pixel(x, y);
                auto it = std::find_if(cache.begin(), cache.end(), [&](auto& e) { return e.first == p; });
                bool ok;
                if (it == cache.end())
                {
                    ok = is_blend(p, n.theme, 2.0);
                    cache.push_back({p, ok});
                    ++distinct;
                    if (!ok) { ++bad_unique; if (!first_bad) first_bad = p; }
                }
                else
                    ok = it->second;
                if (!ok) ++bad;
            }
        std::printf("%-10s distinct colors=%d, off-theme pixels=%d (unique %d)", n.name, distinct, bad, bad_unique);
        if (bad) std::printf(" first bad=#%08X", first_bad);
        std::printf("\n");
        failures += bad;
        images.push_back(img);
    }
    for (size_t i = 0; i < images.size(); ++i)
        for (size_t j = i + 1; j < images.size(); ++j)
        {
            int diff = 0;
            for (int y = 0; y < images[i].height(); ++y)
                for (int x = 0; x < images[i].width(); ++x)
                    diff += images[i].pixel(x, y) != images[j].pixel(x, y);
            std::printf("%s vs %s: %d differing pixels of %d\n", themes[i].name, themes[j].name, diff,
                        images[i].width() * images[i].height());
            if (diff == 0) ++failures;
        }
    // Side-by-side sheet.
    QImage sheet(images[0].width() * 3 + 40, images[0].height() + 20, QImage::Format_ARGB32);
    sheet.fill(Qt::white);
    QPainter sp(&sheet);
    for (size_t i = 0; i < images.size(); ++i)
        sp.drawImage(10 + int(i) * (images[0].width() + 10), 10, images[i]);
    sp.end();
    sheet.save(QString::fromStdString(out + "/sheet.png"));
    std::printf("%s\n", failures ? "PROBE: FAILURES" : "PROBE: OK");
    return failures ? 1 : 0;
}
