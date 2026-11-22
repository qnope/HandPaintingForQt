// usage: alphacheck img [ref]  -> prints count of alpha<255 pixels, and diff count/bbox vs ref
#include <QGuiApplication>
#include <QImage>
#include <cstdio>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QImage a = QImage(argv[1]).convertToFormat(QImage::Format_ARGB32);
    int n = 0, minx = 1 << 30, miny = 1 << 30, maxx = -1, maxy = -1;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (qAlpha(a.pixel(x, y)) < 255) { ++n; minx = std::min(minx, x); miny = std::min(miny, y); maxx = std::max(maxx, x); maxy = std::max(maxy, y); }
    std::printf("%s %dx%d alpha<255: %d", argv[1], a.width(), a.height(), n);
    if (n) std::printf(" bbox (%d,%d)-(%d,%d)", minx, miny, maxx, maxy);
    if (argc > 2) {
        QImage b = QImage(argv[2]).convertToFormat(QImage::Format_ARGB32);
        int d = 0, white = 0;
        for (int y = 0; y < std::min(a.height(), b.height()); ++y)
            for (int x = 0; x < std::min(a.width(), b.width()); ++x) {
                QRgb p = a.pixel(x, y), q = b.pixel(x, y);
                if (p != q) ++d;
                // pixel much lighter than ref (whitish streak indicator)
                if (qGray(p) > qGray(q) + 25 && qGray(p) > 235) ++white;
            }
        std::printf(" | differs from ref: %d px, much-lighter-than-ref & near-white: %d", d, white);
    }
    std::printf("\n");
    return 0;
}
