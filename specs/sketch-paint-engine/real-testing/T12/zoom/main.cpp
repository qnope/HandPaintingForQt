#include <QGuiApplication>
#include <QImage>
// usage: zoom src dst x y w h scale
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QImage img(argv[1]);
    QImage c = img.copy(atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    int s = atoi(argv[7]);
    return c.scaled(c.width() * s, c.height() * s).save(argv[2]) ? 0 : 1;
}
