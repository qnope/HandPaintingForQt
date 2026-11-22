#include <hpq/RoughGeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    hpq::RoughOptions opt; // defaults: roughness 1, stroke_count 2, max_offset 2
    hpq::RoughGeometryEngine eng;
    QImage img(560, 200, QImage::Format_ARGB32); img.fill(QColor(250, 246, 235));
    QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.scale(2, 2);
    QPainterPath ell; ell.addEllipse(10, 10, 80, 40);
    QPainterPath rr; rr.addRoundedRect(100, 10, 100, 60, 12, 12);
    QPainterPath btn; btn.addRoundedRect(210, 10, 60, 24, 4, 4);
    QPainterPath circ; circ.addEllipse(215, 50, 40, 40);
    p.setPen(QPen(QColor(40, 40, 40), 1));
    for (const auto &s : {ell, rr, btn, circ})
        for (const auto &st : eng.outline(s, opt, 7)) p.drawPath(st);
    p.end(); img.save(argv[1]);
}
