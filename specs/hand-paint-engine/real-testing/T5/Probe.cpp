// Real-test probe for hand-paint-engine T5 (B6, B8, B9): uses only the public hpq API.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygon>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

struct Seg { QPointF a, b; };

static double dist_seg(QPointF p, const Seg& s)
{
    QPointF d = s.b - s.a;
    double l2 = d.x() * d.x() + d.y() * d.y();
    double t = l2 == 0 ? 0 : ((p - s.a).x() * d.x() + (p - s.a).y() * d.y()) / l2;
    t = std::clamp(t, 0.0, 1.0);
    QPointF q = s.a + t * d;
    return std::hypot(p.x() - q.x(), p.y() - q.y());
}
static double dist_outline(QPointF p, const std::vector<Seg>& segs)
{
    double m = 1e9;
    for (auto& s : segs) m = std::min(m, dist_seg(p, s));
    return m;
}
static bool is_ink_blend(QColor c, QColor bg, QColor ink)
{
    int dr = ink.red() - bg.red(), dg = ink.green() - bg.green(), db = ink.blue() - bg.blue();
    int big = std::max({std::abs(dr), std::abs(dg), std::abs(db)});
    double t;
    if (big == std::abs(dr)) t = double(c.red() - bg.red()) / dr;
    else if (big == std::abs(dg)) t = double(c.green() - bg.green()) / dg;
    else t = double(c.blue() - bg.blue()) / db;
    if (t < -0.02 || t > 1.02) return false;
    return std::abs(c.red() - (bg.red() + t * dr)) <= 2 && std::abs(c.green() - (bg.green() + t * dg)) <= 2
        && std::abs(c.blue() - (bg.blue() + t * db)) <= 2;
}

struct Case {
    std::string name;
    std::vector<Seg> segs;
    std::function<void(QPainter&)> draw;
};

static std::vector<Seg> poly(const std::vector<QPointF>& pts, bool closed)
{
    std::vector<Seg> s;
    for (size_t i = 0; i + 1 < pts.size(); ++i) s.push_back({pts[i], pts[i + 1]});
    if (closed) s.push_back({pts.back(), pts.front()});
    return s;
}

static QImage zoom(const QImage& img, QRect r, int f)
{
    return img.copy(r).scaled(r.width() * f, r.height() * f, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : "out";
    std::vector<std::pair<std::string, hpq::Theme>> themes = {
        {"sketch", hpq::Theme::sketch()}, {"china_ink", hpq::Theme::china_ink()}, {"chalk", hpq::Theme::chalk()}};

    auto rect_segs = [](QRectF r) {
        return poly({r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft()}, true);
    };
    std::vector<QPointF> tri = {{100, 30}, {170, 170}, {30, 170}};
    std::vector<QPointF> pl = {{30, 30}, {100, 170}, {170, 30}};

    std::vector<Case> cases;
    cases.push_back({"rectF", rect_segs(QRectF(40, 40, 120, 120)), [](QPainter& p) { p.drawRect(QRectF(40, 40, 120, 120)); }});
    cases.push_back({"rectInt", rect_segs(QRectF(40, 40, 120, 120)), [](QPainter& p) { p.drawRect(QRect(40, 40, 120, 120)); }});
    cases.push_back({"rectXYWH", rect_segs(QRectF(40, 40, 120, 120)), [](QPainter& p) { p.drawRect(40, 40, 120, 120); }});
    {
        std::vector<Seg> s;
        QRectF rs[] = {QRectF(20, 20, 60, 50), QRectF(110, 30, 70, 70), QRectF(40, 120, 120, 60)};
        for (auto& r : rs) { auto v = rect_segs(r); s.insert(s.end(), v.begin(), v.end()); }
        cases.push_back({"rectsF", s, [rs](QPainter& p) { p.drawRects(rs, 3); }});
        QRect ri[] = {QRect(20, 20, 60, 50), QRect(110, 30, 70, 70), QRect(40, 120, 120, 60)};
        cases.push_back({"rectsInt", s, [ri](QPainter& p) { p.drawRects(ri, 3); }});
    }
    cases.push_back({"polygonF", poly(tri, true), [tri](QPainter& p) { p.drawPolygon(QPolygonF(QVector<QPointF>(tri.begin(), tri.end()))); }});
    cases.push_back({"polygonInt", poly(tri, true), [](QPainter& p) { p.drawPolygon(QPolygon({QPoint(100, 30), QPoint(170, 170), QPoint(30, 170)})); }});
    cases.push_back({"polygonWinding", poly(tri, true), [tri](QPainter& p) { p.drawPolygon(tri.data(), 3, Qt::WindingFill); }});
    cases.push_back({"polylineF", poly(pl, false), [pl](QPainter& p) { p.drawPolyline(pl.data(), 3); }});
    cases.push_back({"polylineInt", poly(pl, false), [](QPainter& p) { p.drawPolyline(QPolygon({QPoint(30, 30), QPoint(100, 170), QPoint(170, 30)})); }});

    int failures = 0;
    for (auto& [tname, theme] : themes) {
        const double D = theme.roughness() + theme.stroke_width();
        for (auto& c : cases) {
            QImage img(200, 200, QImage::Format_ARGB32);
            img.fill(Qt::red);
            {
                hpq::HandPaintDevice dev(img, theme);
                QPainter p(&dev);
                p.setPen(QPen(Qt::red, 7));
                p.setBrush(Qt::NoBrush);
                c.draw(p);
            }
            QImage plain(200, 200, QImage::Format_ARGB32);
            plain.fill(theme.background());
            {
                QPainter p(&plain);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(theme.ink(), theme.stroke_width()));
                p.setBrush(Qt::NoBrush);
                c.draw(p);
            }
            const QColor bg = theme.background(), ink = theme.ink();
            int non_bg = 0, beyond_D = 0, beyond_D1 = 0, not_blend = 0;
            double max_d = 0;
            for (int y = 0; y < 200; ++y)
                for (int x = 0; x < 200; ++x) {
                    QColor px = img.pixelColor(x, y);
                    if (px.rgb() == bg.rgb()) continue;
                    ++non_bg;
                    double d = dist_outline(QPointF(x + 0.5, y + 0.5), c.segs);
                    max_d = std::max(max_d, d);
                    if (d > D) ++beyond_D;
                    if (d > D + 1.0) ++beyond_D1;
                    if (!is_ink_blend(px, bg, ink)) ++not_blend;
                }
            // coverage: every sample point of ideal outline has an ink-blend non-bg pixel within D
            int uncovered = 0, samples = 0;
            QPointF worst;
            for (auto& s : c.segs) {
                double len = std::hypot(s.b.x() - s.a.x(), s.b.y() - s.a.y());
                int n = int(len * 2);
                for (int i = 0; i <= n; ++i) {
                    QPointF q = s.a + (s.b - s.a) * (double(i) / n);
                    ++samples;
                    bool ok = false;
                    int r = int(std::ceil(D)) + 1;
                    for (int y = int(q.y()) - r; y <= int(q.y()) + r && !ok; ++y)
                        for (int x = int(q.x()) - r; x <= int(q.x()) + r && !ok; ++x) {
                            if (x < 0 || y < 0 || x >= 200 || y >= 200) continue;
                            QColor px = img.pixelColor(x, y);
                            if (px.rgb() == bg.rgb()) continue;
                            if (std::hypot(x + 0.5 - q.x(), y + 0.5 - q.y()) <= D && is_ink_blend(px, bg, ink)) ok = true;
                        }
                    if (!ok) { ++uncovered; worst = q; }
                }
            }
            const bool differs = img != plain;
            std::string extra;
            bool ok = beyond_D == 0 && uncovered == 0 && differs && not_blend == 0;
            if (c.name.rfind("rect", 0) == 0 && c.name != "rectsF" && c.name != "rectsInt") {
                bool center_bg = img.pixelColor(100, 100).rgb() == bg.rgb();
                extra += center_bg ? " center(100,100)=bg" : " center(100,100)=NOT-bg";
                ok = ok && center_bg;
            }
            if (c.name.rfind("polyline", 0) == 0) {
                // ink along the top chord (30,30)-(170,30), farther than D from both segments
                int bad = 0;
                for (int x = 0; x < 200; ++x)
                    for (int y = 20; y <= 40; ++y) {
                        QColor px = img.pixelColor(x, y);
                        if (px.rgb() == bg.rgb()) continue;
                        if (dist_outline(QPointF(x + 0.5, y + 0.5), c.segs) > D) ++bad;
                    }
                bool p100 = img.pixelColor(100, 30).rgb() == bg.rgb();
                extra += " closingChordInk=" + std::to_string(bad) + (p100 ? " (100,30)=bg" : " (100,30)=NOT-bg");
                ok = ok && bad == 0 && p100;
            }
            if (!ok) ++failures;
            std::printf("%-9s %-14s D=%.1f nonbg=%5d maxDist=%.2f beyondD=%d beyondD+1=%d notInkBlend=%d uncovered=%d/%d%s differsFromPlain=%d%s -> %s\n",
                        tname.c_str(), c.name.c_str(), D, non_bg, max_d, beyond_D, beyond_D1, not_blend, uncovered, samples,
                        uncovered ? (" (e.g. " + std::to_string(worst.x()) + "," + std::to_string(worst.y()) + ")").c_str() : "",
                        int(differs), extra.c_str(), ok ? "OK" : "FAIL");
            const QString base = out + "/" + QString::fromStdString(tname + "_" + c.name);
            img.save(base + ".png");
            plain.save(base + "_plain.png");
            img.scaled(600, 600, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(base + "_x3.png");
            if (c.name == "rectF" || c.name == "rectInt") {
                QImage corners(4 * 160, 160, QImage::Format_ARGB32);
                QPainter cp(&corners);
                QRect cr[] = {QRect(30, 30, 20, 20), QRect(150, 30, 20, 20), QRect(150, 150, 20, 20), QRect(30, 150, 20, 20)};
                for (int i = 0; i < 4; ++i) cp.drawImage(i * 160, 0, zoom(img, cr[i], 8));
                cp.end();
                corners.save(base + "_corners_x8.png");
            }
            if (c.name.rfind("polygon", 0) == 0) {
                QImage corners(3 * 160, 160, QImage::Format_ARGB32);
                QPainter cp(&corners);
                QRect cr[] = {QRect(90, 20, 20, 20), QRect(160, 160, 20, 20), QRect(20, 160, 20, 20)};
                for (int i = 0; i < 3; ++i) cp.drawImage(i * 160, 0, zoom(img, cr[i], 8));
                cp.end();
                corners.save(base + "_corners_x8.png");
            }
            if (c.name.rfind("polyline", 0) == 0) {
                QImage corners(3 * 160, 160, QImage::Format_ARGB32);
                QPainter cp(&corners);
                QRect cr[] = {QRect(20, 20, 20, 20), QRect(90, 160, 20, 20), QRect(160, 20, 20, 20)};
                for (int i = 0; i < 3; ++i) cp.drawImage(i * 160, 0, zoom(img, cr[i], 8));
                cp.end();
                corners.save(base + "_corners_x8.png");
            }
        }
    }
    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
