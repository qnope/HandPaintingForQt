// Real-test probe for T4 (B5): draws lines with a red pen on a HandPaintDevice
// and checks/visualises the result like a user would.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QLineF>
#include <QPainter>
#include <QPen>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

struct Named
{
    const char* name;
    hpq::Theme theme;
};

double dist_to_segment(QPointF p, QLineF s)
{
    const QPointF d = s.p2() - s.p1();
    const double len2 = d.x() * d.x() + d.y() * d.y();
    double t = len2 > 0 ? ((p.x() - s.x1()) * d.x() + (p.y() - s.y1()) * d.y()) / len2 : 0.0;
    t = std::clamp(t, 0.0, 1.0);
    const QPointF q = s.p1() + t * d;
    return std::hypot(p.x() - q.x(), p.y() - q.y());
}

bool is_ink_blend(QColor c, QColor bg, QColor ink)
{
    const int bgc[3] = {bg.red(), bg.green(), bg.blue()};
    const int inc[3] = {ink.red(), ink.green(), ink.blue()};
    const int cc[3] = {c.red(), c.green(), c.blue()};
    int k = 0;
    for (int i = 1; i < 3; ++i)
        if (std::abs(inc[i] - bgc[i]) > std::abs(inc[k] - bgc[k])) k = i;
    const double span = inc[k] - bgc[k];
    double t = span != 0 ? (cc[k] - bgc[k]) / span : 0.0;
    if (t < -0.02 || t > 1.02) return false;
    t = std::clamp(t, 0.0, 1.0);
    for (int i = 0; i < 3; ++i) {
        const double expected = bgc[i] + t * (inc[i] - bgc[i]);
        if (std::abs(cc[i] - expected) > 2.0) return false;
    }
    return true;
}

QImage upscale_crop(const QImage& img, QRect r, int factor)
{
    return img.copy(r).scaled(r.width() * factor, r.height() * factor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";

    const std::vector<Named> themes = {{"sketch", hpq::Theme::sketch()},
                                       {"china_ink", hpq::Theme::china_ink()},
                                       {"chalk", hpq::Theme::chalk()}};
    struct Seg
    {
        const char* name;
        QLineF line;
    };
    const std::vector<Seg> segs = {{"horizontal", QLineF(20, 100, 180, 100)},
                                   {"diagonal", QLineF(30, 170, 170, 30)},
                                   {"vertical", QLineF(100, 20, 100, 180)},
                                   {"reverse_shallow", QLineF(180, 120, 20, 80)}};

    bool all_ok = true;
    for (const auto& nt : themes) {
        const hpq::Theme& th = nt.theme;
        const double D = th.roughness() + th.stroke_width();
        QImage sheet(4 * 200 + 3 * 10, 200, QImage::Format_ARGB32);
        sheet.fill(Qt::gray);
        int col = 0;
        for (const auto& sg : segs) {
            QImage img(200, 200, QImage::Format_ARGB32);
            img.fill(Qt::red);
            {
                hpq::HandPaintDevice dev(img, th);
                QPainter p(&dev);
                p.setPen(QPen(Qt::red, 1.0));
                p.setBrush(Qt::NoBrush);
                p.drawLine(sg.line.p1(), sg.line.p2());
                p.end();
            }
            QImage plain(200, 200, QImage::Format_ARGB32);
            plain.fill(th.background());
            {
                QPainter p(&plain);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(th.ink(), th.stroke_width()));
                p.drawLine(sg.line);
            }

            // Checks
            int non_bg = 0, not_blend = 0, red_like = 0, far = 0, far_square = 0, diff_plain = 0;
            double max_d = 0;
            std::vector<QPoint> blend_px;
            for (int y = 0; y < 200; ++y)
                for (int x = 0; x < 200; ++x) {
                    const QColor c = img.pixelColor(x, y);
                    if (img.pixel(x, y) != plain.pixel(x, y)) ++diff_plain;
                    if (c == th.background()) continue;
                    ++non_bg;
                    if (!is_ink_blend(c, th.background(), th.ink())) {
                        ++not_blend;
                        if (c.red() > std::max(th.background().red(), th.ink().red()) + 2) ++red_like;
                    } else {
                        blend_px.push_back(QPoint(x, y));
                    }
                    const double d = dist_to_segment(QPointF(x + 0.5, y + 0.5), sg.line);
                    max_d = std::max(max_d, d);
                    if (d > D) {
                        ++far;
                        std::printf("  pixel (%d,%d) %s center-dist %.2f (square-dist %.2f) D %.2f\n", x, y,
                                    qPrintable(c.name()), d, d - std::sqrt(0.5), D);
                    }
                    if (d - std::sqrt(0.5) > D) ++far_square;
                }
            // coverage: each point on the segment, sampled every pixel
            const int n = int(std::ceil(sg.line.length()));
            int uncovered = 0;
            double worst_cov = 0;
            for (int i = 0; i <= n; ++i) {
                const QPointF q = sg.line.pointAt(double(i) / n);
                double best = 1e9;
                for (const QPoint& b : blend_px)
                    best = std::min(best, std::hypot(b.x() + 0.5 - q.x(), b.y() + 0.5 - q.y()));
                worst_cov = std::max(worst_cov, best);
                if (best > D) ++uncovered;
            }
            // Count separate ink runs across the stroke at the middle (evidence of several strokes)
            std::string runs;
            if (std::string(sg.name) == "horizontal") {
                for (int x : {40, 100, 160}) {
                    int r = 0;
                    bool in = false;
                    for (int y = 90; y < 110; ++y) {
                        const QColor c = img.pixelColor(x, y);
                        const int dv = std::abs(c.lightness() - th.background().lightness());
                        const bool ink = dv > std::abs(th.ink().lightness() - th.background().lightness()) / 2;
                        if (ink && !in) ++r;
                        in = ink;
                    }
                    runs += " x" + std::to_string(x) + ":" + std::to_string(r);
                }
            }
            const bool ok = non_bg > 0 && not_blend == 0 && far_square == 0 && uncovered == 0 && diff_plain > 0;
            all_ok = all_ok && ok;
            std::printf("[%s %s] D=%.2f non_bg=%d not_blend=%d red_like=%d far_center(>D)=%d far_square(>D)=%d max_center_dist=%.2f "
                        "uncovered=%d worst_cov=%.2f diff_vs_plain=%d strong-ink-runs%s -> %s\n",
                        nt.name, sg.name, D, non_bg, not_blend, red_like, far, far_square, max_d, uncovered, worst_cov,
                        diff_plain, runs.c_str(), ok ? "OK" : "FAIL");

            const std::string base = out + "/" + nt.name + "_" + sg.name;
            img.save(QString::fromStdString(base + ".png"));
            plain.save(QString::fromStdString(base + "_plain.png"));
            {
                QPainter sp(&sheet);
                sp.drawImage(col * 210, 0, img);
            }
            ++col;
            if (std::string(sg.name) == "horizontal") {
                upscale_crop(img, QRect(15, 90, 70, 20), 12).save(QString::fromStdString(base + "_crop_left_x12.png"));
                upscale_crop(img, QRect(115, 90, 70, 20), 12).save(QString::fromStdString(base + "_crop_right_x12.png"));
                upscale_crop(plain, QRect(15, 90, 70, 20), 12).save(QString::fromStdString(base + "_plain_crop_left_x12.png"));
            }
            if (std::string(sg.name) == "diagonal")
                upscale_crop(img, QRect(25, 120, 50, 55), 10).save(QString::fromStdString(base + "_crop_x10.png"));
            if (std::string(sg.name) == "vertical")
                upscale_crop(img, QRect(90, 15, 20, 60), 10).save(QString::fromStdString(base + "_crop_x10.png"));
        }
        sheet.scaled(sheet.width() * 2, sheet.height() * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation)
            .save(QString::fromStdString(out + "/" + nt.name + "_sheet_x2.png"));
    }

    // Determinism check of the default seed (informational, B18 is a later task)
    {
        QImage a(200, 200, QImage::Format_ARGB32), b(200, 200, QImage::Format_ARGB32);
        for (QImage* im : {&a, &b}) {
            hpq::HandPaintDevice dev(*im, hpq::Theme::sketch());
            QPainter p(&dev);
            p.setPen(QPen(Qt::red));
            p.drawLine(QPointF(20, 100), QPointF(180, 100));
        }
        std::printf("[info] two default-seed devices identical: %s\n", a == b ? "yes" : "no");
    }
    std::printf("PROBE: %s\n", all_ok ? "ALL OK" : "SOME FAIL");
    return all_ok ? 0 : 1;
}
