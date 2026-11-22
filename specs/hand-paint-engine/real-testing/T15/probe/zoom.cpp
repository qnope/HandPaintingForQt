#include <QGuiApplication>
#include <QImage>
#include <cstdlib>
// zoom in out x y w h scale
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QImage img(argv[1]);
    QImage c = img.copy(atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    int s = atoi(argv[7]);
    c.scaled(c.width() * s, c.height() * s, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(argv[2]);
}
