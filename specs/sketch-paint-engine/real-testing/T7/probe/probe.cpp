#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>
#include <cstdio>

static QPolygonF star(QPointF c, double r) {
    QPolygonF p;
    for (int i = 0; i < 5; ++i) {
        double a = -M_PI / 2 + i * 4 * M_PI / 5;
        p << QPointF(c.x() + r * std::cos(a), c.y() + r * std::sin(a));
    }
    return p;
}

// Draws the scene on any painter (device painter or plain reference painter).
static void scene(QPainter &p) {
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(255, 200, 120));
    // Row 1: lines
    p.drawLine(QLine(20, 20, 180, 20));
    p.drawLine(QLineF(20.5, 40.5, 180.5, 80.5));
    QLineF ls[2] = {QLineF(20, 90, 180, 90), QLineF(100, 60, 100, 110)};
    p.drawLines(ls, 2);
    // rect + rounded rect
    p.drawRect(QRect(210, 20, 120, 80));
    p.drawRoundedRect(QRectF(350, 20, 120, 80), 20, 20);
    // ellipse
    p.setBrush(QColor(150, 200, 255));
    p.drawEllipse(QRectF(490, 20, 140, 90));
    // Row 2: stars
    p.setBrush(QColor(255, 120, 120));
    p.drawPolygon(star(QPointF(100, 210), 80), Qt::OddEvenFill);
    p.drawPolygon(star(QPointF(280, 210), 80), Qt::WindingFill);
    // convex polygon
    p.setBrush(QColor(140, 220, 140));
    QPolygonF hex;
    for (int i = 0; i < 6; ++i) hex << QPointF(450 + 70 * std::cos(i * M_PI / 3), 210 + 70 * std::sin(i * M_PI / 3));
    p.drawConvexPolygon(hex);
    // polyline with brush set: must stay open and unfilled
    p.setBrush(QColor(255, 0, 255));
    QPolygonF pl; pl << QPointF(560, 280) << QPointF(600, 140) << QPointF(640, 280);
    p.drawPolyline(pl);
    // Row 3: path with curve, closed, and open path
    p.setBrush(QColor(220, 180, 255));
    QPainterPath path;
    path.moveTo(30, 330);
    path.cubicTo(80, 260, 160, 400, 210, 330);
    path.lineTo(210, 400);
    path.lineTo(30, 400);
    path.closeSubpath();
    p.drawPath(path);
    QPainterPath open;
    open.moveTo(250, 400);
    open.cubicTo(300, 300, 380, 450, 440, 330);
    p.setBrush(Qt::NoBrush);
    p.drawPath(open);
    // points in QPoint int polygon form
    p.setBrush(QColor(255, 255, 150));
    QPolygon tri; tri << QPoint(480, 400) << QPoint(540, 320) << QPoint(600, 400);
    p.drawPolygon(tri);
}

static QImage render(bool hand) {
    QImage img(660, 430, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    if (hand) {
        hpq::SketchTheme theme;
        hpq::RoughGeometryEngine ge;
        hpq::HandPaintDevice dev(target, theme, ge);
        QPainter p(&dev);
        scene(p);
        p.end();
    } else {
        scene(target);
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
    // QGraphicsScene rendered onto the device painter
    QGraphicsScene gs(0, 0, 400, 250);
    gs.setBackgroundBrush(Qt::white);
    gs.addRect(20, 20, 150, 100, QPen(Qt::darkBlue, 2), QColor(180, 220, 255));
    gs.addEllipse(200, 30, 160, 110, QPen(Qt::darkRed, 2), QColor(255, 210, 200));
    gs.addLine(20, 200, 380, 160, QPen(Qt::black, 3));
    QPolygonF tri; tri << QPointF(60, 230) << QPointF(120, 150) << QPointF(180, 230);
    gs.addPolygon(tri, QPen(Qt::darkGreen, 2), QColor(200, 255, 200));
    for (bool h : {false, true}) {
        QImage img(400, 250, QImage::Format_ARGB32);
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
    }
    std::puts("done");
    return 0;
}
