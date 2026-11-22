#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPainter>
#include <cstdio>
#include <functional>

using Fn = std::function<void(QPainter&)>;
static const QRgb Mag = qRgb(255,0,255), Cya = qRgb(0,255,255);

static QPixmap make_pixmap() {
    QImage im(60, 40, QImage::Format_RGB32);
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 60; ++x) im.setPixel(x, y, x < 30 ? Mag : Cya);
    return QPixmap::fromImage(im);
}
// render with device (hand=true) or plain painter
static QImage render(int w, int h, bool hand, const Fn &target_setup, const Fn &draw) {
    QImage img(w, h, QImage::Format_RGB32); img.fill(Qt::white);
    QPainter target(&img);
    target.setRenderHint(QPainter::Antialiasing);
    target_setup(target);
    if (hand) {
        hpq::SketchTheme theme; hpq::RoughGeometryEngine geo;
        hpq::HandPaintDevice device(target, theme, geo);
        QPainter p(&device); draw(p); p.end();
    } else {
        target.save(); draw(target); target.restore();
    }
    target.end();
    return img;
}
static QRect bounds_if(const QImage &img, const std::function<bool(QRgb)> &pred, QRect area = QRect()) {
    if (area.isNull()) area = img.rect();
    QRect r;
    for (int y = area.top(); y <= area.bottom(); ++y) for (int x = area.left(); x <= area.right(); ++x)
        if (pred(img.pixel(x, y))) r |= QRect(x, y, 1, 1);
    return r;
}
static bool is_mag(QRgb c) { return c == Mag; }
static bool is_cya(QRgb c) { return c == Cya; }
static bool is_pix(QRgb c) { return c == Mag || c == Cya; }
static bool is_blue(QRgb c) { return qBlue(c) > qRed(c) + 60 && qBlue(c) > qGreen(c) + 60; }
static bool non_white(QRgb c) { return c != qRgb(255,255,255); }
static int count_diff(const QImage &a, const QImage &b, const std::function<bool(QRgb)> &pred) {
    int n = 0;
    for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x)
        if (pred(a.pixel(x,y)) != pred(b.pixel(x,y))) ++n;
    return n;
}
static int count_outside(const QImage &img, const QRect &allowed) {
    int n = 0;
    for (int y = 0; y < img.height(); ++y) for (int x = 0; x < img.width(); ++x)
        if (!allowed.contains(x,y) && non_white(img.pixel(x,y))) ++n;
    return n;
}
static void pr(const char *what, QRect r) { std::printf("  %-34s (%d,%d %dx%d)\n", what, r.x(), r.y(), r.width(), r.height()); }

static void pair(const char *name, int w, int h, const Fn &ts, const Fn &draw, QRect text_area = QRect()) {
    QImage hand = render(w, h, true, ts, draw), plain = render(w, h, false, ts, draw);
    hand.save(QString("../%1_hand.png").arg(name)); plain.save(QString("../%1_plain.png").arg(name));
    std::printf("%s\n", name);
    pr("pixmap bounds hand", bounds_if(hand, is_pix)); pr("pixmap bounds plain", bounds_if(plain, is_pix));
    pr("magenta bounds hand", bounds_if(hand, is_mag)); pr("magenta bounds plain", bounds_if(plain, is_mag));
    std::printf("  pixmap-color pixel mask differences = %d (mag %d, cya %d)\n",
                count_diff(hand, plain, is_pix), count_diff(hand, plain, is_mag), count_diff(hand, plain, is_cya));
    pr("blue text bounds hand", bounds_if(hand, is_blue, text_area)); pr("blue text bounds plain", bounds_if(plain, is_blue, text_area));
    pr("all ink hand", bounds_if(hand, non_white)); pr("all ink plain", bounds_if(plain, non_white));
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const QPixmap pm = make_pixmap();
    auto none = [](QPainter&) {};
    QFont f("Helvetica"); f.setPixelSize(24);

    // 1. QGraphicsScene
    QGraphicsScene scene(0, 0, 600, 400);
    scene.addRect(QRectF(30, 30, 120, 80), QPen(Qt::black, 2), QBrush(QColor(220, 80, 80)));
    scene.addEllipse(QRectF(200, 40, 140, 90), QPen(Qt::black, 2), QBrush(QColor(80, 200, 80)));
    scene.addPolygon(QPolygonF({QPointF(60,200), QPointF(160,180), QPointF(130,320), QPointF(40,300)}), QPen(Qt::black, 2), QBrush(QColor(240, 200, 60)));
    auto *t = scene.addSimpleText("Hello scene", f); t->setBrush(QColor(0,0,255)); t->setPos(200, 240);
    auto *pi = scene.addPixmap(pm); pi->setPos(400, 170);
    pair("scene", 600, 400, none, [&](QPainter &p) { scene.render(&p, QRectF(0,0,600,400), QRectF(0,0,600,400)); });
    std::printf("  expected: pixmap (400,170 60x40); text near (200,240)\n");

    // 2. translate+scale with pixmap and text
    pair("ts_text_pixmap", 400, 300, none, [&](QPainter &p) {
        p.translate(100, 50); p.scale(2, 2);
        p.drawPixmap(QPointF(0, 0), pm);                // expect (100,50 120x80)
        p.setPen(QColor(0,0,255)); p.setFont(f);
        p.drawText(QPointF(0, 80), "Scaled");           // baseline at device y=210
    });
    std::printf("  expected: pixmap (100,50 120x80); text baseline y=210 x>=100, ~48px glyphs\n");

    // 3. rotate with pixmap, image, tiled pixmap and text
    pair("rotate_text_pixmap", 400, 400, none, [&](QPainter &p) {
        p.translate(200, 200); p.rotate(30);
        p.drawPixmap(QPointF(-30, -20), pm);
        p.drawImage(QPointF(40, -20), pm.toImage());
        p.drawTiledPixmap(QRectF(-100, 60, 150, 40), pm.scaled(20, 20));
        p.setPen(QColor(0,0,255)); p.setFont(f);
        p.drawText(QPointF(-60, -50), "Rotated");
    });

    // 4. clip with text and pixmaps
    pair("clip_text_pixmap", 400, 300, none, [&](QPainter &p) {
        p.setClipRect(QRect(50, 50, 200, 100));
        p.drawPixmap(QRectF(20, 20, 120, 80), pm, QRectF());       // partly outside
        p.drawTiledPixmap(QRectF(180, 60, 200, 200), pm.scaled(20, 20));
        p.setPen(QColor(0,0,255)); QFont big("Helvetica"); big.setPixelSize(60); p.setFont(big);
        p.drawText(QPointF(100, 170), "Clipped text");
    });
    {
        QImage hand = render(400, 300, true, none, [&](QPainter &p) {
            p.setClipRect(QRect(50, 50, 200, 100));
            p.drawPixmap(QRectF(20, 20, 120, 80), pm, QRectF());
            p.drawTiledPixmap(QRectF(180, 60, 200, 200), pm.scaled(20, 20));
            p.setPen(QColor(0,0,255)); QFont big("Helvetica"); big.setPixelSize(60); p.setFont(big);
            p.drawText(QPointF(100, 170), "Clipped text");
        });
        std::printf("  hand ink outside clip (50,50,200,100) = %d\n", count_outside(hand, QRect(50,50,200,100)));
    }

    // 5. translate+scale+rotate+clip together
    pair("trs_clip_text_pixmap", 400, 400, none, [&](QPainter &p) {
        p.translate(80, 60); p.scale(1.5, 1.5);
        p.setClipRect(QRect(0, 0, 160, 120));               // device (80,60,240,180)
        p.rotate(15);
        p.drawPixmap(QPointF(20, 10), pm);
        p.setPen(QColor(0,0,255)); p.setFont(f);
        p.drawText(QPointF(10, 110), "Transformed and clipped long text");
        p.setPen(QPen(Qt::black, 2)); p.setBrush(QColor(200,60,60));
        p.drawRect(QRectF(100, 20, 80, 50));
    });
    {
        QImage hand = render(400, 400, true, none, [&](QPainter &p) {
            p.translate(80, 60); p.scale(1.5, 1.5); p.setClipRect(QRect(0, 0, 160, 120)); p.rotate(15);
            p.drawPixmap(QPointF(20, 10), pm);
            p.setPen(QColor(0,0,255)); p.setFont(f); p.drawText(QPointF(10, 110), "Transformed and clipped long text");
            p.setPen(QPen(Qt::black, 2)); p.setBrush(QColor(200,60,60)); p.drawRect(QRectF(100, 20, 80, 50));
        });
        std::printf("  hand ink outside clip (80,60,240,180) = %d\n", count_outside(hand, QRect(80,60,240,180)));
    }

    // 6. target painter with its own offset (and clip), device draws text/pixmap with transform+clip
    auto target_off = [](QPainter &t) { t.translate(60, 40); t.setClipRect(QRect(0, 0, 300, 220)); };
    auto draw6 = [&](QPainter &p) {
        p.drawPixmap(QPointF(0, 0), pm);                    // expect (60,40)
        p.save(); p.translate(100, 0); p.scale(2, 2);
        p.drawPixmap(QPointF(0, 0), pm);                    // expect (160,40 120x80)
        p.restore();
        p.setPen(QColor(0,0,255)); p.setFont(f);
        p.drawText(QPointF(0, 120), "Offset target");       // baseline y=160
        p.setClipRect(QRect(0, 140, 100, 60));              // device (60,180,100,60)
        p.drawTiledPixmap(QRectF(-50, 100, 400, 200), pm.scaled(20, 20));
        p.setPen(QPen(Qt::black, 2)); p.setBrush(Qt::black); p.drawRect(QRectF(-50, 100, 400, 200));
    };
    pair("target_offset_text_pixmap", 400, 300, target_off, draw6, QRect(0, 0, 400, 175));
    {
        QImage hand = render(400, 300, true, target_off, draw6);
        std::printf("  hand ink outside union((60,40,220,100) text/pixmaps, (54,34,..)) check: ink in y>240 = %d, ink below 175 outside (60,180,100,60) = %d\n",
            [&]{int n=0; for(int y=241;y<300;++y) for(int x=0;x<400;++x) if(non_white(hand.pixel(x,y))) ++n; return n;}(),
            [&]{int n=0; QRect c(60,180,100,60); for(int y=176;y<300;++y) for(int x=0;x<400;++x) if(!c.contains(x,y)&&non_white(hand.pixel(x,y))) ++n; return n;}());
    }
    return 0;
}
