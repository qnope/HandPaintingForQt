// montage <out.png> <a.png> <b.png>: side by side, scaled 2x, with labels.
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QImage a(argv[2]), b(argv[3]);
    QImage out(a.width() * 2 + 10, a.height() + 20, QImage::Format_ARGB32);
    out.fill(Qt::white);
    QPainter p(&out);
    p.drawText(5, 14, QString::fromLocal8Bit(argv[2]));
    p.drawText(a.width() + 15, 14, QString::fromLocal8Bit(argv[3]));
    p.drawImage(0, 20, a);
    p.drawImage(a.width() + 10, 20, b);
    p.end();
    out.scaled(out.size() * 2).save(argv[1]);
}
