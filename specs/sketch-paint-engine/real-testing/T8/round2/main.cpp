#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cstdio>

class IdentityEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &s, const hpq::RoughOptions &, std::uint32_t) const override { return {s}; }
    QPainterPath fill_area(const QPainterPath &s, const hpq::RoughOptions &, std::uint32_t) const override { return s; }
};

static QImage blank(int w = 240, int h = 160) { QImage img(w, h, QImage::Format_ARGB32); img.fill(Qt::white); return img; }

template <class F>
static QImage via_device(const hpq::GeometryEngine &ge, const hpq::Theme &theme, F draw, int w = 240, int h = 160) {
    QImage img = blank(w, h);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    hpq::HandPaintDevice dev(target, theme, ge);
    QPainter p(&dev);
    draw(p);
    p.end();
    target.end();
    return img;
}

static int diff_pixels(const QImage &a, const QImage &b) {
    int n = 0;
    for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x) if (a.pixel(x, y) != b.pixel(x, y)) ++n;
    return n;
}
static int nonwhite(const QImage &a) {
    int n = 0;
    for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x) if (a.pixel(x, y) != qRgb(255,255,255)) ++n;
    return n;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    hpq::SketchTheme theme;
    QPen pen(Qt::darkBlue, 3); QBrush brush(QColor(255, 180, 0));

    // B15: identity engine vs direct Qt
    IdentityEngine id;
    auto drawA = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.drawRect(QRectF(10, 10, 100, 50)); p.drawRect(QRectF(130, 70, 80, 60)); };
    QImage viaId = via_device(id, theme, drawA);
    QImage direct = blank();
    { QPainter p(&direct); p.setRenderHint(QPainter::Antialiasing); drawA(p); }
    viaId.save("b15_identity_device.png"); direct.save("b15_direct_qt.png");
    std::printf("B15 identity vs direct: %d differing pixels, %d non-white pixels in identity image\n", diff_pixels(viaId, direct), nonwhite(viaId));

    // B15 with transform: identity engine + translate vs direct Qt with translate
    auto drawF = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.translate(100, 60); p.drawRect(QRectF(10, 10, 100, 50)); };
    QImage idT = via_device(id, theme, drawF);
    QImage dirT = blank();
    { QPainter p(&dirT); p.setRenderHint(QPainter::Antialiasing); drawF(p); }
    idT.save("b15_identity_translated_device.png"); dirT.save("b15_translated_direct_qt.png");
    std::printf("B15 identity+translate(100,60) vs direct Qt: %d differing pixels\n", diff_pixels(idT, dirT));

    // B16: rough engine, two sessions
    hpq::RoughGeometryEngine rough;
    auto drawB = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.drawRect(QRectF(20, 20, 160, 100)); };
    auto drawC = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.drawRect(QRectF(40, 30, 150, 110)); };
    QImage r1 = via_device(rough, theme, drawB);
    QImage r2 = via_device(rough, theme, drawB);
    QImage r3 = via_device(rough, theme, drawC);
    r1.save("b16_rough_session1.png"); r2.save("b16_rough_session2.png"); r3.save("b16_rough_other_rect.png");
    std::printf("B16 session1 vs session2: %d differing pixels (non-white %d)\n", diff_pixels(r1, r2), nonwhite(r1));
    std::printf("B16 rect B vs rect C: %d differing pixels\n", diff_pixels(r1, r3));

    // Translate(100,100)+rect(10,10) vs rect(110,110) without transform
    auto drawT = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.translate(100, 100); p.drawRect(QRectF(10, 10, 100, 60)); };
    auto drawR = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.drawRect(QRectF(110, 110, 100, 60)); };
    QImage rt = via_device(rough, theme, drawT, 260, 220);
    QImage rr = via_device(rough, theme, drawR, 260, 220);
    rt.save("b16_translate100_rect10.png"); rr.save("b16_rect110_no_transform.png");
    std::printf("translate(100,100)+rect(10,10) vs rect(110,110): %d differing pixels (non-white %d / %d)\n",
                diff_pixels(rt, rr), nonwhite(rt), nonwhite(rr));

    // Whole scene, two renders
    auto scene = [&](QPainter &p) {
        p.setPen(pen); p.setBrush(brush);
        p.drawRect(QRectF(15, 15, 120, 70));
        p.setBrush(QColor(120, 200, 255));
        p.drawRect(QRectF(160, 20, 100, 90));
        p.setBrush(QColor(140, 220, 140));
        p.drawEllipse(QRectF(30, 120, 140, 80));
        p.setBrush(QColor(240, 120, 160));
        QPolygonF poly; poly << QPointF(200, 140) << QPointF(290, 130) << QPointF(270, 210) << QPointF(210, 200);
        p.drawPolygon(poly);
        p.setPen(Qt::black);
        p.drawText(QPointF(20, 240), QStringLiteral("Hello sketch 123"));
        p.save(); p.translate(150, 230); p.rotate(-10); p.setPen(pen); p.setBrush(Qt::NoBrush); p.drawRect(QRectF(0, 0, 120, 30)); p.restore();
    };
    QImage s1 = via_device(rough, theme, scene, 320, 280);
    QImage s2 = via_device(rough, theme, scene, 320, 280);
    s1.save("scene_render1.png"); s2.save("scene_render2.png");
    std::printf("Scene render1 vs render2: %d differing pixels (non-white %d)\n", diff_pixels(s1, s2), nonwhite(s1));
    return 0;
}
