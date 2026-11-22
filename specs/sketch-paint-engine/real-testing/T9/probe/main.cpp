#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <cstdio>
#include <functional>

static bool dark(const QImage &img, int x, int y) { return qGray(img.pixel(x, y)) < 128; }
static bool white(const QImage &img, int x, int y) { return img.pixel(x, y) == qRgb(255,255,255); }
static QRect ink_bounds(const QImage &img) {
    QRect r;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) != qRgb(255,255,255)) r |= QRect(x, y, 1, 1);
    return r;
}
static int ink_outside(const QImage &img, const QRect &allowed) {
    int n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (!allowed.contains(x, y) && img.pixel(x, y) != qRgb(255,255,255)) ++n;
    return n;
}
static QImage render(int w, int h, const std::function<void(QPainter&)> &target_setup,
                     const std::function<void(QPainter&)> &draw) {
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    target_setup(target);
    hpq::SketchTheme theme;
    hpq::RoughGeometryEngine geo;
    hpq::HandPaintDevice device(target, theme, geo);
    QPainter p(&device);
    draw(p);
    p.end();
    target.end();
    return img;
}
static void report(const char *name, const QImage &img) {
    img.save(QString("../%1.png").arg(name));
    QRect b = ink_bounds(img);
    std::printf("%-28s ink bounds = (%d,%d %dx%d)\n", name, b.x(), b.y(), b.width(), b.height());
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    auto none = [](QPainter&) {};

    // B17 a: exact test-file scenario
    QImage a = render(300, 300, none, [](QPainter &p) {
        p.translate(100, 100); p.scale(2, 2); p.setPen(Qt::NoPen);
        p.fillRect(QRectF(0, 0, 20, 20), Qt::black);
    });
    report("b17_translate_scale_fill", a);
    std::printf("  (120,120) dark=%d  (20,20) white=%d  ink outside (96,96,48,48)=%d\n",
                dark(a,120,120), white(a,20,20), ink_outside(a, QRect(96,96,48,48)));

    // B17 b: pen + brush rect, translate+scale+rotate, plus a path; reference untransformed rect too
    QImage b = render(400, 400, none, [](QPainter &p) {
        p.setPen(QPen(Qt::black, 2)); p.setBrush(QColor(200, 60, 60));
        p.drawRect(QRectF(10, 10, 60, 40));            // reference, identity
        p.save();
        p.translate(250, 120); p.rotate(30); p.scale(1.5, 1.5);
        p.drawRect(QRectF(-30, -20, 60, 40));          // rotated, scaled
        p.restore();
        p.save();
        p.translate(80, 300); p.scale(2, 1);
        QPainterPath path; path.moveTo(0,0); path.cubicTo(30,-80, 60,80, 90,0); path.closeSubpath();
        p.setBrush(QColor(60, 60, 200));
        p.drawPath(path);
        p.restore();
        p.drawRect(QRectF(330, 330, 50, 50));          // after restore: identity again
    });
    report("b17_rotate_scale_path_saverestore", b);

    // B17 c: nested save/restore
    QImage c = render(300, 300, none, [](QPainter &p) {
        p.setPen(QPen(Qt::black, 2)); p.setBrush(Qt::NoBrush);
        p.save(); p.translate(50, 50);
          p.drawRect(0, 0, 40, 40);                    // at 50,50
          p.save(); p.translate(100, 0); p.scale(2, 2);
            p.drawRect(0, 0, 40, 40);                  // at 150,50 size 80
          p.restore();
          p.drawRect(0, 100, 40, 40);                  // at 50,150
        p.restore();
        p.drawRect(200, 200, 40, 40);                  // at 200,200
    });
    report("b17_nested_save_restore", c);

    // B18 a: exact test-file scenario
    QImage d = render(300, 300, none, [](QPainter &p) {
        p.setClipRect(QRect(0, 0, 50, 50)); p.setPen(Qt::NoPen);
        p.fillRect(QRectF(0, 0, 200, 200), Qt::black);
    });
    report("b18_cliprect_fill", d);
    std::printf("  (25,25) dark=%d  ink outside (0,0,50,50)=%d\n", dark(d,25,25), ink_outside(d, QRect(0,0,50,50)));

    // B18 b: clip rect with pen outline, and clip under translate
    QImage e = render(300, 300, none, [](QPainter &p) {
        p.translate(100, 100);
        p.setClipRect(QRect(0, 0, 100, 60));            // device (100,100,100,60)
        p.setPen(QPen(Qt::black, 4)); p.setBrush(QColor(60, 160, 60));
        p.drawRect(QRectF(-50, -50, 200, 200));
        p.drawEllipse(QRectF(20, 10, 60, 40));
    });
    report("b18_cliprect_translated_pen", e);
    std::printf("  ink outside (100,100,100,60)=%d\n", ink_outside(e, QRect(100,100,100,60)));

    // B18 c: clip path (circle)
    QImage f = render(300, 300, none, [](QPainter &p) {
        QPainterPath circle; circle.addEllipse(QPointF(150, 150), 80, 80);
        p.setClipPath(circle);
        p.setPen(QPen(Qt::black, 3)); p.setBrush(QColor(200, 120, 40));
        p.drawRect(QRectF(20, 20, 260, 260));
    });
    report("b18_clippath_circle", f);
    int outside_circle = 0;
    for (int y = 0; y < 300; ++y) for (int x = 0; x < 300; ++x) {
        double dx = x + 0.5 - 150, dy = y + 0.5 - 150;
        if (dx*dx + dy*dy > 82*82 && f.pixel(x,y) != qRgb(255,255,255)) ++outside_circle;
    }
    std::printf("  ink outside circle r=82 = %d\n", outside_circle);

    // B17+B18 with target painter already translated (widget offset 60,40) and clipped
    QImage g = render(300, 300, [](QPainter &t) { t.translate(60, 40); },
        [](QPainter &p) {
            p.setPen(QPen(Qt::black, 2)); p.setBrush(QColor(60, 60, 200));
            p.drawRect(QRectF(0, 0, 50, 50));            // expect at (60,40)
            p.save(); p.translate(100, 0); p.scale(2, 2);
            p.drawRect(QRectF(0, 0, 30, 30));            // expect (160,40,60,60)
            p.restore();
            p.setClipRect(QRect(0, 100, 60, 60));        // device (60,140,60,60)
            p.setBrush(Qt::black);
            p.drawRect(QRectF(-100, 80, 300, 300));
        });
    report("target_offset_transform_clip", g);
    std::printf("  ink outside (54,34,170,170)=%d ; ink in y>200 =%d ; ink x>124 & y>136 = %d\n",
        ink_outside(g, QRect(54,34,170,170)),
        [&]{int n=0; for(int y=201;y<300;++y) for(int x=0;x<300;++x) if(g.pixel(x,y)!=qRgb(255,255,255)) ++n; return n;}(),
        [&]{int n=0; for(int y=137;y<300;++y) for(int x=125;x<300;++x) if(g.pixel(x,y)!=qRgb(255,255,255)) ++n; return n;}());

    // target painter with its own clip (widget clip) + device clip
    QImage h = render(300, 300, [](QPainter &t) { t.translate(50, 50); t.setClipRect(QRect(0,0,200,200)); },
        [](QPainter &p) {
            p.setClipRect(QRect(100, 100, 150, 150));    // device clip; intersection with target clip -> target (150,150,100,100)
            p.setPen(Qt::NoPen);
            p.fillRect(QRectF(-50, -50, 400, 400), Qt::black);
        });
    report("target_clip_and_device_clip", h);
    std::printf("  ink bounds expected (150,150 100x100); ink outside=%d\n", ink_outside(h, QRect(150,150,100,100)));
    return 0;
}
