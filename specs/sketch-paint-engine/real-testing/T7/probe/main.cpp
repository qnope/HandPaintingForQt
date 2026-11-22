#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/GeometryEngine.h>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static QPolygonF star(QPointF c, double r) {
    QPolygonF p;
    for (int i = 0; i < 5; ++i) {
        double a = -M_PI / 2 + i * 4 * M_PI / 5;
        p << QPointF(c.x() + r * std::cos(a), c.y() + r * std::sin(a));
    }
    return p;
}

// Spy wrapping the real engine: logs each outline()/fill_area() call.
struct Spy : hpq::GeometryEngine {
    hpq::RoughGeometryEngine real;
    mutable std::vector<std::string> log;
    std::vector<QPainterPath> outline(const QPainterPath &s, const hpq::RoughOptions &o, std::uint32_t seed) const override {
        QRectF b = s.boundingRect();
        char buf[200];
        std::snprintf(buf, sizeof buf, "outline   bbox=(%.1f,%.1f %.1fx%.1f) elems=%d fillrule=%s", b.x(), b.y(), b.width(), b.height(),
                      s.elementCount(), s.fillRule() == Qt::WindingFill ? "Winding" : "OddEven");
        log.push_back(buf);
        return real.outline(s, o, seed);
    }
    QPainterPath fill_area(const QPainterPath &s, const hpq::RoughOptions &o, std::uint32_t seed) const override {
        QRectF b = s.boundingRect();
        char buf[200];
        std::snprintf(buf, sizeof buf, "fill_area bbox=(%.1f,%.1f %.1fx%.1f) fillrule=%s", b.x(), b.y(), b.width(), b.height(),
                      s.fillRule() == Qt::WindingFill ? "Winding" : "OddEven");
        log.push_back(buf);
        return real.fill_area(s, o, seed);
    }
};

static void scene(QPainter &p, std::vector<std::string> *log, const Spy *spy) {
    auto mark = [&](const char *name) { if (log) { log->push_back(std::string("== ") + name); } (void)spy; };
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(255, 200, 120));
    mark("drawLine(QLine 20,20-180,20)");
    p.drawLine(QLine(20, 20, 180, 20));
    mark("drawLine(QLineF 20.5,40.5-180.5,80.5)");
    p.drawLine(QLineF(20.5, 40.5, 180.5, 80.5));
    mark("drawLines(2)");
    QLineF ls[2] = {QLineF(20, 90, 180, 90), QLineF(100, 60, 100, 110)};
    p.drawLines(ls, 2);
    mark("drawRect(210,20 120x80)");
    p.drawRect(QRect(210, 20, 120, 80));
    mark("drawRoundedRect(350,20 120x80)");
    p.drawRoundedRect(QRectF(350, 20, 120, 80), 20, 20);
    p.setBrush(QColor(150, 200, 255));
    mark("drawEllipse(490,20 140x90)");
    p.drawEllipse(QRectF(490, 20, 140, 90));
    p.setBrush(QColor(255, 120, 120));
    mark("drawPolygon star OddEven (c=100,210 r=80)");
    p.drawPolygon(star(QPointF(100, 210), 80), Qt::OddEvenFill);
    mark("drawPolygon star Winding (c=280,210 r=80)");
    p.drawPolygon(star(QPointF(280, 210), 80), Qt::WindingFill);
    p.setBrush(QColor(140, 220, 140));
    QPolygonF hex;
    for (int i = 0; i < 6; ++i) hex << QPointF(450 + 70 * std::cos(i * M_PI / 3), 210 + 70 * std::sin(i * M_PI / 3));
    mark("drawConvexPolygon hexagon (380,149.4 140x121.2)");
    p.drawConvexPolygon(hex);
    p.setBrush(QColor(255, 0, 255));
    QPolygonF pl; pl << QPointF(560, 280) << QPointF(600, 140) << QPointF(640, 280);
    mark("drawPolyline (560,140 80x140) brush magenta set");
    p.drawPolyline(pl);
    p.setBrush(QColor(220, 180, 255));
    QPainterPath path;
    path.moveTo(30, 330);
    path.cubicTo(80, 260, 160, 400, 210, 330);
    path.lineTo(210, 400);
    path.lineTo(30, 400);
    path.closeSubpath();
    mark("drawPath closed with cubic");
    p.drawPath(path);
    QPainterPath open;
    open.moveTo(250, 400);
    open.cubicTo(300, 300, 380, 450, 440, 330);
    p.setBrush(Qt::NoBrush);
    mark("drawPath open cubic NoBrush");
    p.drawPath(open);
    p.setBrush(QColor(255, 255, 150));
    QPolygon tri; tri << QPoint(480, 400) << QPoint(540, 320) << QPoint(600, 400);
    mark("drawPolygon int triangle (480,320 120x80)");
    p.drawPolygon(tri);
}

static QImage render(bool hand, Spy *spy = nullptr) {
    QImage img(660, 430, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    if (hand) {
        hpq::SketchTheme theme;
        hpq::RoughGeometryEngine rge;
        const hpq::GeometryEngine &ge = spy ? static_cast<const hpq::GeometryEngine &>(*spy) : rge;
        hpq::HandPaintDevice dev(target, theme, ge);
        QPainter p(&dev);
        scene(p, spy ? &spy->log : nullptr, spy);
        p.end();
    } else {
        scene(target, nullptr, nullptr);
    }
    target.end();
    return img;
}

static void px(const QImage &img, const char *what, int x, int y) {
    QColor c = img.pixelColor(x, y);
    std::printf("  %-34s (%3d,%3d) = rgb(%d,%d,%d)\n", what, x, y, c.red(), c.green(), c.blue());
}

static int count_color(const QImage &img, QRect r, QColor want) {
    int n = 0;
    for (int y = r.top(); y <= r.bottom(); ++y)
        for (int x = r.left(); x <= r.right(); ++x) {
            QColor c = img.pixelColor(x, y);
            if (std::abs(c.red() - want.red()) < 20 && std::abs(c.green() - want.green()) < 20 && std::abs(c.blue() - want.blue()) < 20) ++n;
        }
    return n;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QImage ref = render(false), hand = render(true), hand2 = render(true);
    ref.save("reference.png");
    hand.save("hand.png");
    std::printf("hand drawn twice identical: %s\n", hand == hand2 ? "yes" : "no");
    std::printf("hand differs from reference: %s\n", hand != ref ? "yes" : "no");
    for (auto *img : {&ref, &hand}) {
        std::printf("%s:\n", img == &ref ? "reference" : "hand");
        px(*img, "oddeven star center (hole)", 100, 215);
        px(*img, "winding star center (filled)", 280, 215);
        px(*img, "polyline inside triangle area", 600, 240);
        std::printf("  magenta px in polyline region: %d\n", count_color(*img, QRect(550, 130, 100, 160), QColor(255, 0, 255)));
        px(*img, "rect center", 270, 60);
        px(*img, "ellipse center", 560, 65);
        px(*img, "hexagon center", 450, 210);
        px(*img, "path center", 120, 370);
        px(*img, "int triangle center", 540, 380);
    }
    // Spy run: what reaches the geometry engine, per primitive.
    Spy spy;
    QImage spied = render(true, &spy);
    std::printf("spy run identical to plain hand run: %s\n", spied == hand ? "yes" : "no");
    std::puts("spy log:");
    for (auto &l : spy.log) std::printf("  %s\n", l.c_str());

    // QGraphicsScene with shapes, text and a pixmap item.
    QGraphicsScene gs(0, 0, 520, 300);
    gs.setBackgroundBrush(Qt::white);
    gs.addRect(20, 20, 150, 100, QPen(Qt::darkBlue, 2), QColor(180, 220, 255));
    gs.addEllipse(200, 30, 160, 110, QPen(Qt::darkRed, 2), QColor(255, 210, 200));
    gs.addLine(20, 200, 380, 160, QPen(Qt::black, 3));
    QPolygonF tri; tri << QPointF(60, 230) << QPointF(120, 150) << QPointF(180, 230);
    gs.addPolygon(tri, QPen(Qt::darkGreen, 2), QColor(200, 255, 200));
    QFont f("Helvetica"); f.setPixelSize(20);
    auto *t = gs.addSimpleText("Hello scene text", f);
    t->setPos(200, 240);
    t->setBrush(Qt::darkMagenta);
    QPixmap pm(100, 60);
    pm.fill(QColor(30, 160, 60));
    { QPainter pp(&pm); pp.setPen(QPen(Qt::yellow, 4)); pp.drawLine(0, 0, 100, 60); pp.drawLine(0, 60, 100, 0); }
    gs.addPixmap(pm)->setPos(400, 170);
    QImage sh, sr;
    for (bool h : {false, true}) {
        QImage img(520, 300, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter target(&img);
        target.setRenderHint(QPainter::Antialiasing);
        if (h) {
            hpq::SketchTheme theme;
            hpq::RoughGeometryEngine ge;
            hpq::HandPaintDevice dev(target, theme, ge);
            QPainter p(&dev);
            gs.render(&p);
            p.end();
        } else {
            gs.render(&target);
        }
        target.end();
        img.save(h ? "scene_hand.png" : "scene_reference.png");
        (h ? sh : sr) = img;
    }
    std::printf("scene pixmap region identical hand vs reference: %s\n",
                sh.copy(400, 170, 100, 60) == sr.copy(400, 170, 100, 60) ? "yes" : "no");
    px(sh, "scene pixmap center", 450, 200);
    // Direct check: translated painter + drawPixmap / drawText at origin.
    {
        QImage img(520, 300, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter target(&img);
        hpq::SketchTheme theme;
        hpq::RoughGeometryEngine ge;
        hpq::HandPaintDevice dev(target, theme, ge);
        QPainter p(&dev);
        p.translate(400, 170);
        p.drawPixmap(QPointF(0, 0), pm);
        p.setPen(Qt::darkMagenta);
        p.setFont(f);
        p.translate(-200, 80);
        p.drawText(QPointF(0, 0), "Translated text");
        p.end();
        target.end();
        img.save("translated_pixmap_text_hand.png");
        px(img, "translated pixmap center (450,200)", 450, 200);
        px(img, "untranslated pixmap spot (50,30)", 50, 30);
    }
    std::puts("done");
    return 0;
}
