#include <hpq/RoughGeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cmath>
#include <cstdio>

static double dist(QPointF a, QPointF b) { return std::hypot(a.x() - b.x(), a.y() - b.y()); }

static QPointF first_pt(const QPainterPath &p) { auto e = p.elementAt(0); return {e.x, e.y}; }
static QPointF last_pt(const QPainterPath &p) { auto e = p.elementAt(p.elementCount() - 1); return {e.x, e.y}; }
static int subpaths(const QPainterPath &p) { int n = 0; for (int i = 0; i < p.elementCount(); ++i) if (p.elementAt(i).isMoveTo()) ++n; return n; }

static bool probe(const char *name, const QPainterPath &shape, bool closed, std::uint32_t seed, const QString &out_dir) {
    hpq::RoughOptions opt; opt.roughness = 1; opt.stroke_count = 2; opt.max_offset = 5;
    hpq::RoughGeometryEngine eng;
    auto strokes = eng.outline(shape, opt, seed);
    bool ok = strokes.size() == 2;
    QRectF bb = shape.boundingRect();
    QRectF big = bb.adjusted(-5, -5, 5, 5);
    std::printf("%s seed=%u: %zu strokes, shape bbox (%.1f,%.1f %.1fx%.1f)\n", name, seed, strokes.size(), bb.x(), bb.y(), bb.width(), bb.height());
    for (size_t i = 0; i < strokes.size(); ++i) {
        const auto &s = strokes[i];
        QRectF sb = s.boundingRect();
        QPointF a = first_pt(s), b = last_pt(s);
        bool inside = big.contains(sb);
        std::printf("  stroke %zu: elements=%d subpaths=%d start=(%.2f,%.2f) end=(%.2f,%.2f) start-end=%.2f bbox=(%.2f,%.2f)-(%.2f,%.2f) inside=%d\n",
                    i, s.elementCount(), subpaths(s), a.x(), a.y(), b.x(), b.y(), dist(a, b), sb.left(), sb.top(), sb.right(), sb.bottom(), inside);
        if (!inside) ok = false;
        if (closed) { if (dist(a, b) > 5) ok = false; }
        else {
            double ds = dist(a, first_pt(shape)), de = dist(b, last_pt(shape));
            std::printf("    open: start->path start=%.2f end->path end=%.2f\n", ds, de);
            if (ds > 5 || de > 5 || dist(a, b) <= 5) ok = false;
        }
    }
    // Render: original faint, strokes in dark graphite, scaled x3
    QRectF view = big.adjusted(-10, -10, 10, 10);
    const double k = 4.0;
    QImage img(int(view.width() * k), int(view.height() * k), QImage::Format_ARGB32);
    img.fill(QColor(250, 246, 235));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(k, k); p.translate(-view.topLeft());
    p.setPen(QPen(QColor(200, 80, 80, 90), 0.5, Qt::DashLine)); p.setBrush(Qt::NoBrush);
    p.drawRect(big);
    if (closed) {
        QPainterPath fa = eng.fill_area(shape, opt, seed);
        std::printf("  fill_area: elements=%d bbox inside=%d contains center=%d\n", fa.elementCount(), big.contains(fa.boundingRect()), fa.contains(bb.center()));
        p.setPen(Qt::NoPen); p.setBrush(QColor(255, 220, 120, 150)); p.drawPath(fa); p.setBrush(Qt::NoBrush);
    }
    p.setPen(QPen(QColor(80, 140, 220, 140), 1.0)); p.drawPath(shape);
    QColor cols[] = {QColor(40, 40, 40), QColor(90, 60, 30)};
    for (size_t i = 0; i < strokes.size(); ++i) { p.setPen(QPen(cols[i % 2], 0.8)); p.drawPath(strokes[i]); }
    p.end();
    QString f = out_dir + "/" + name + ".png";
    img.save(f);
    std::printf("  -> %s %s\n", qPrintable(f), ok ? "OK" : "VIOLATION");
    return ok;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QString out = argc > 1 ? argv[1] : ".";
    QPainterPath rect; rect.addRect(10, 10, 100, 50);
    QPainterPath ell; ell.addEllipse(0, 0, 80, 40);
    QPainterPath tri; tri.addPolygon(QPolygonF({{0, 0}, {100, 0}, {50, 80}})); tri.closeSubpath();
    QPainterPath rr; rr.addRoundedRect(0, 0, 100, 60, 12, 12);
    QPainterPath cub; cub.moveTo(0, 0); cub.cubicTo(60, -40, 120, 40, 100, 80); cub.cubicTo(60, 120, -20, 60, 0, 0); cub.closeSubpath();
    QPainterPath open; open.moveTo(0, 0); open.cubicTo(30, -40, 70, 40, 100, 0);
    // A bigger triangle without closeSubpath (polygon as a user would build it with QPolygonF closed)
    QPainterPath star; QPolygonF sp; for (int i = 0; i < 10; ++i) { double r = i % 2 ? 30 : 80, a = i * M_PI / 5 - M_PI / 2; sp << QPointF(100 + r * std::cos(a), 100 + r * std::sin(a)); } sp << sp.first(); star.addPolygon(sp);
    int bad = 0;
    for (std::uint32_t seed : {1u, 42u, 12345u}) {
        QString suffix = QString("_s%1").arg(seed);
        bad += !probe(qPrintable("rectangle" + suffix), rect, true, seed, out);
        bad += !probe(qPrintable("ellipse" + suffix), ell, true, seed, out);
        bad += !probe(qPrintable("triangle" + suffix), tri, true, seed, out);
        bad += !probe(qPrintable("roundedrect" + suffix), rr, true, seed, out);
        bad += !probe(qPrintable("closedcubic" + suffix), cub, true, seed, out);
        bad += !probe(qPrintable("star" + suffix), star, true, seed, out);
        bad += !probe(qPrintable("opencubic" + suffix), open, false, seed, out);
    }
    std::printf("violations: %d\n", bad);
    return bad ? 1 : 0;
}
