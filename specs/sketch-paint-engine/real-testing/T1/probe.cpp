#include <hpq/RoughGeometryEngine.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <cstdio>
#include <algorithm>

static double dist_seg(QPointF p) { // distance to segment (0,0)-(100,0)
    double x = std::clamp(p.x(), 0.0, 100.0);
    return std::hypot(p.x() - x, p.y());
}
static double dist(QPointF a, QPointF b) { return std::hypot(a.x()-b.x(), a.y()-b.y()); }

static void report(const char *name, const std::vector<QPainterPath> &strokes) {
    std::printf("== %s: %zu strokes\n", name, strokes.size());
    for (size_t i = 0; i < strokes.size(); ++i) {
        const auto &s = strokes[i];
        if (s.elementCount() == 0) { std::printf("  stroke %zu: empty\n", i); continue; }
        double maxd = 0;
        for (int k = 0; k < s.elementCount(); ++k) maxd = std::max(maxd, dist_seg(s.elementAt(k)));
        // also flattened polygon
        double maxflat = 0;
        for (const auto &poly : s.toSubpathPolygons()) for (auto p : poly) maxflat = std::max(maxflat, dist_seg(p));
        QPointF first = s.elementAt(0), last = s.elementAt(s.elementCount()-1);
        std::printf("  stroke %zu: elements=%d first=(%.3f,%.3f) last=(%.3f,%.3f) dFirst=%.3f dLast=%.3f maxDistElem=%.3f maxDistFlat=%.3f\n",
            i, s.elementCount(), first.x(), first.y(), last.x(), last.y(),
            dist(first, {0,0}), dist(last, {100,0}), maxd, maxflat);
    }
}

static void render(const char *file, const std::vector<std::pair<std::vector<QPainterPath>, QColor>> &sets, double m) {
    QImage img(700, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(50, 150); p.scale(6, 6); // 100px -> 600px
    // tolerance band
    p.setPen(Qt::NoPen); p.setBrush(QColor(255, 220, 220));
    QPainterPath band; band.addRoundedRect(QRectF(-m, -m, 100 + 2*m, 2*m), m, m); p.drawPath(band);
    p.setPen(QPen(QColor(180,180,180), 0.15, Qt::DashLine)); p.drawLine(QPointF(0,0), QPointF(100,0));
    for (const auto &[strokes, color] : sets) {
        p.setPen(QPen(color, 0.35)); p.setBrush(Qt::NoBrush);
        for (const auto &s : strokes) p.drawPath(s);
    }
    p.end();
    img.save(file);
}

int main(int argc, char **argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    QGuiApplication app(argc, argv);
    hpq::RoughGeometryEngine eng;
    const hpq::GeometryEngine &ge = eng; // use through interface
    QPainterPath line; line.moveTo(0,0); line.lineTo(100,0);

    hpq::RoughOptions o; o.roughness = 1; o.stroke_count = 2; o.max_offset = 5;
    auto b1 = ge.outline(line, o, 42);
    report("B1 seed42 m=5", b1);
    render("b1_seed42.png", {{b1, Qt::black}}, 5);

    hpq::RoughOptions o2 = o; o2.max_offset = 15;
    auto b1b = ge.outline(line, o2, 7);
    report("B1 seed7 m=15", b1b);
    render("b1_seed7_m15.png", {{b1b, Qt::black}}, 15);

    auto s1 = ge.outline(line, o, 1), s2 = ge.outline(line, o, 2);
    report("B3 seed1", s1); report("B3 seed2", s2);
    std::printf("B3 seed1 == seed2 (first stroke): %s\n", s1[0] == s2[0] ? "EQUAL" : "DIFFERENT");
    render("b3_seeds1_2.png", {{s1, Qt::blue}, {s2, Qt::red}}, 5);
    auto again = ge.outline(line, o, 1);
    std::printf("seed1 repeat equal: %s\n", again[0] == s1[0] && again[1] == s1[1] ? "yes" : "no");

    hpq::RoughOptions o0 = o; o0.roughness = 0;
    auto r0 = ge.outline(line, o0, 42);
    report("B4 roughness0", r0);
    render("b4_roughness0.png", {{r0, Qt::black}}, 5);

    // edge cases a user might try
    hpq::RoughOptions o3 = o; o3.stroke_count = 5;
    report("stroke_count 5", ge.outline(line, o3, 3));
    QPainterPath diag; diag.moveTo(10,10); diag.lineTo(10,10);
    report("zero-length line", ge.outline(diag, o, 3));
    report("empty path", ge.outline(QPainterPath(), o, 3));
    return 0;
}
