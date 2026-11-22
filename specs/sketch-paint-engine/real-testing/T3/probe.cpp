#include <hpq/RoughGeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cstdio>

using namespace hpq;

static bool same(const QPainterPath &a, const QPainterPath &b) {
    if (a.elementCount() != b.elementCount()) return false;
    for (int i = 0; i < a.elementCount(); ++i) {
        auto x = a.elementAt(i), y = b.elementAt(i);
        if (x.type != y.type || x.x != y.x || x.y != y.y) return false;
    }
    return true;
}

static int check(const char *name, const QPainterPath &shape, QPointF center, double m, const char *png) {
    RoughGeometryEngine eng;
    RoughOptions o; o.roughness = 1; o.max_offset = m; o.stroke_count = 2;
    int bad = 0;
    QPainterPath f = eng.fill_area(shape, o, 7);
    QPainterPath f2 = eng.fill_area(shape, o, 7);
    auto ol = eng.outline(shape, o, 7);
    auto ol2 = eng.outline(shape, o, 7);
    int n = f.elementCount();
    int moveTos = 0;
    for (int i = 0; i < n; ++i) if (f.elementAt(i).type == QPainterPath::MoveToElement) ++moveTos;
    QPointF first = f.elementAt(0), last = f.elementAt(n - 1);
    QRectF bb = f.boundingRect();
    QRectF lim = shape.boundingRect().adjusted(-m, -m, m, m);
    bool closed = first == last;
    bool inside = lim.contains(bb);
    bool ctr = f.contains(center);
    bool exact = same(f, shape);
    bool detF = same(f, f2);
    bool detO = ol.size() == ol2.size();
    for (size_t i = 0; detO && i < ol.size(); ++i) detO = same(ol[i], ol2[i]);
    printf("[%s] fill elements=%d subpaths=%d first=(%.3f,%.3f) last=(%.3f,%.3f) closed=%d\n", name, n, moveTos,
           first.x(), first.y(), last.x(), last.y(), closed);
    printf("[%s] fill bbox=(%.3f,%.3f %.3fx%.3f) limit=(%.3f,%.3f %.3fx%.3f) inside=%d contains_center=%d identical_to_shape=%d\n",
           name, bb.x(), bb.y(), bb.width(), bb.height(), lim.x(), lim.y(), lim.width(), lim.height(), inside, ctr, exact);
    printf("[%s] determinism fill=%d outline=%d (outline strokes=%zu)\n", name, detF, detO, ol.size());
    QPainterPath fs9 = eng.fill_area(shape, o, 9);
    printf("[%s] seed 9 fill differs from seed 7: %d\n", name, !same(fs9, f));
    bad += !closed + !inside + !ctr + !detF + !detO + (moveTos != 1);

    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(3, 3);
    p.translate(10, 10);
    p.fillPath(f, QColor(255, 200, 80));
    p.setPen(QPen(QColor(0, 160, 0), 0.3, Qt::DashLine));
    p.drawRect(lim);
    p.setPen(QPen(Qt::blue, 0.3));
    p.drawPath(shape);
    p.setPen(QPen(Qt::black, 0.6));
    for (auto &s : ol) p.drawPath(s);
    p.setPen(QPen(Qt::red, 1));
    p.drawPoint(center);
    p.end();
    img.save(png);
    return bad;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QPainterPath rect; rect.addRect(10, 10, 100, 50);
    QPainterPath ell; ell.addEllipse(0, 0, 80, 40);
    int bad = check("rect", rect, {60, 35}, 5, "rect.png");
    bad += check("ellipse", ell, {40, 20}, 5, "ellipse.png");
    // large max_offset on small shape
    QPainterPath small; small.addRect(0, 0, 20, 20);
    bad += check("small-rect-offset8", small, {10, 10}, 8, "small.png");
    printf("bad=%d\n", bad);
    return bad;
}
