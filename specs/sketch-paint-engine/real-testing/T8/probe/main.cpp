#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cstdio>

class IdentityEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &s, const hpq::RoughOptions &, std::uint32_t) const override { return {s}; }
    QPainterPath fill_area(const QPainterPath &s, const hpq::RoughOptions &, std::uint32_t) const override { return s; }
};

static QImage blank() { QImage img(240, 160, QImage::Format_ARGB32); img.fill(Qt::white); return img; }

template <class F>
static QImage via_device(const hpq::GeometryEngine &ge, const hpq::Theme &theme, F draw) {
    QImage img = blank();
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

    // Same rect drawn via translated transform vs raw coordinates (same device coords)
    auto drawD = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.translate(20, 20); p.drawRect(QRectF(0, 0, 160, 100)); };
    QImage r4 = via_device(rough, theme, drawD);
    r4.save("b16_rough_translated.png");
    std::printf("B16 rect B vs same device rect via translate: %d differing pixels\n", diff_pixels(r1, r4));

    // Same size rect at another place: wobble shape compared after shifting
    auto drawE = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.drawRect(QRectF(21, 20, 160, 100)); };
    QImage r5 = via_device(rough, theme, drawE);
    QImage r5s = r5.copy(1, 0, 240, 160);
    std::printf("B16 rect B vs rect shifted 1px (re-aligned): %d differing pixels\n", diff_pixels(r1, r5s));
    // Identity engine with a painter transform vs direct Qt
    auto drawF = [&](QPainter &p) { p.setPen(pen); p.setBrush(brush); p.translate(100, 60); p.drawRect(QRectF(10, 10, 100, 50)); };
    QImage idT = via_device(id, theme, drawF);
    QImage dirT = blank();
    { QPainter p(&dirT); p.setRenderHint(QPainter::Antialiasing); drawF(p); }
    idT.save("other_identity_translated_device.png"); dirT.save("other_translated_direct_qt.png");
    std::printf("Identity engine with translate(100,60) vs direct Qt: %d differing pixels\n", diff_pixels(idT, dirT));
    return 0;
}
