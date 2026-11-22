// usage: region img x y w h -> distinct colors histogram (top 5) inside rect
#include <QGuiApplication>
#include <QImage>
#include <QMap>
#include <cstdio>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QImage a = QImage(argv[1]).convertToFormat(QImage::Format_ARGB32);
    int x0 = atoi(argv[2]), y0 = atoi(argv[3]), w = atoi(argv[4]), h = atoi(argv[5]);
    QMap<QRgb, int> m;
    for (int y = y0; y < y0 + h; ++y) for (int x = x0; x < x0 + w; ++x) m[a.pixel(x, y)]++;
    QList<QPair<int, QRgb>> v; for (auto it = m.begin(); it != m.end(); ++it) v.append({it.value(), it.key()});
    std::sort(v.begin(), v.end(), [](auto &p, auto &q) { return p.first > q.first; });
    std::printf("%s rect(%d,%d,%d,%d): %lld colors;", argv[1], x0, y0, w, h, (long long)v.size());
    for (int i = 0; i < std::min<int>(5, v.size()); ++i) std::printf(" #%08x x%d", v[i].second, v[i].first);
    std::printf("\n");
}
