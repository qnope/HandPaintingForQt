// Diagnostic: which text paths reach the target through a HandPaintDevice,
// and crops (x3) of the themed window regions where text is missing.
#include <hpq/HandPaintDevice.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QTextLayout>
#include <QTextDocument>
#include <QStaticText>
#include <cstdio>
#include <QLineEdit>
#include <QLabel>

static int ink(const QImage &img) {
    int n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (qGray(img.pixel(x, y)) < 200) ++n;
    return n;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    hpq::SketchTheme theme;
    hpq::RoughGeometryEngine engine;
    QFont f("Helvetica", 16);
    auto run = [&](const char *name, auto fn) {
        QImage img(300, 60, QImage::Format_ARGB32);
        img.fill(Qt::white);
        {
            QPainter target(&img);
            hpq::HandPaintDevice dev(target, theme, engine);
            QPainter p(&dev);
            p.setFont(f);
            p.setPen(Qt::black);
            fn(p);
        }
        img.save(QString("out/textpath_%1.png").arg(name));
        std::printf("%-12s dark pixels: %d\n", name, ink(img));
    };
    run("drawText", [&](QPainter &p) { p.drawText(10, 40, "Hello text"); });
    run("QTextLayout", [&](QPainter &p) {
        QTextLayout l("Hello text", f);
        l.beginLayout(); auto line = l.createLine(); line.setLineWidth(280); l.endLayout();
        l.draw(&p, QPointF(10, 10));
    });
    run("QTextDoc", [&](QPainter &p) {
        QTextDocument d; d.setDefaultFont(f); d.setPlainText("Hello text"); d.drawContents(&p);
    });
    run("StaticText", [&](QPainter &p) { p.drawStaticText(10, 10, QStaticText("Hello text")); });

    run("clip_text", [&](QPainter &p) { p.setClipRect(0, 0, 300, 60); p.drawText(10, 40, "Hello text"); });
    run("tr_clip_text", [&](QPainter &p) { p.translate(5, 5); p.setClipRect(0, 0, 290, 50); p.drawText(10, 35, "Hello text"); });
    run("clip_rect", [&](QPainter &p) { p.setClipRect(0, 0, 300, 60); p.drawRect(10, 10, 100, 30); });
    run("brush_text", [&](QPainter &p) { p.setPen(QPen(QBrush(Qt::black), 1)); p.setBrush(Qt::NoBrush); p.drawText(10, 40, "Hello text"); });
    QImage themed("out/themed.png");
    struct C { const char *n; QRect r; } crops[] = {
        {"lineedit_combo", {0, 210, 360, 70}}, {"table", {360, 25, 360, 130}},
        {"tree", {360, 225, 360, 100}}, {"groupbox", {0, 280, 360, 120}},
        {"scene", {440, 430, 200, 90}}};
    QImage plain("out/plain_fusion_sketchpalette.png");
    for (auto &c : crops) {
        themed.copy(c.r).scaled(c.r.size() * 2).save(QString("out/crop_themed_%1.png").arg(c.n));
        plain.copy(c.r).scaled(c.r.size() * 2).save(QString("out/crop_plain_%1.png").arg(c.n));
    }
    for (int k = 0; k < 2; ++k) {
        QWidget *w = k ? (QWidget *)new QLineEdit("Line edit text") : (QWidget *)new QLabel("Label text");
        w->resize(200, 30);
        w->setPalette(theme.palette());
        QImage img(200, 30, QImage::Format_ARGB32); img.fill(Qt::white);
        { QPainter target(&img); hpq::HandPaintDevice dev(target, theme, engine); QPainter p(&dev); w->render(&p); }
        img.save(QString("out/render_%1.png").arg(k ? "lineedit" : "label"));
        std::printf("render %-9s dark pixels: %d\n", k ? "lineedit" : "label", ink(img));
    }
    for (int off : {0, 20, 60}) {
        QWidget parent; parent.resize(260, 120); parent.setPalette(theme.palette());
        auto *le = new QLineEdit("Line edit text", &parent); le->setGeometry(off, off, 200, 30);
        QImage img(260, 120, QImage::Format_ARGB32); img.fill(Qt::white);
        { QPainter target(&img); hpq::HandPaintDevice dev(target, theme, engine); QPainter p(&dev); parent.render(&p); }
        img.save(QString("out/render_child_lineedit_off%1.png").arg(off));
        std::printf("child lineedit at offset %2d dark pixels: %d\n", off, ink(img));
    }
    return 0;
}
