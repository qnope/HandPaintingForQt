#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QTest>
#include <QSet>
#include <cstdio>

class RectWidget : public QWidget {
public:
    RectWidget() { resize(100, 80); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setPen(QPen(Qt::black));
        p.setBrush(Qt::NoBrush);
        p.drawRect(10, 10, 80, 60);
    }
};

static void report(const char* name, const QPixmap& pm, const QString& out) {
    QImage img = pm.toImage();
    qreal dpr = pm.devicePixelRatio();
    std::printf("%s: pixmap %dx%d dpr=%.1f logical %.0fx%.0f\n", name, img.width(), img.height(), dpr,
                img.width() / dpr, img.height() / dpr);
    auto px = [&](int x, int y) { return img.pixelColor(int(x * dpr), int(y * dpr)).name(); };
    std::printf("  center(50,40)=%s corner(2,2)=%s (95,75)=%s\n", qPrintable(px(50, 40)), qPrintable(px(2, 2)),
                qPrintable(px(95, 75)));
    // bounding box of non-background pixels in logical coordinates
    QColor bg = img.pixelColor(int(50 * dpr), int(40 * dpr));
    int minx = 1e9, miny = 1e9, maxx = -1, maxy = -1, n = 0;
    QSet<QRgb> colors;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x) {
            QColor c = img.pixelColor(x, y);
            colors.insert(c.rgba());
            if (c != bg) { ++n; minx = std::min(minx, x); miny = std::min(miny, y); maxx = std::max(maxx, x); maxy = std::max(maxy, y); }
        }
    std::printf("  non-bg pixels=%d bbox(logical)=(%.1f,%.1f)-(%.1f,%.1f) distinct colors=%d\n", n, minx / dpr,
                miny / dpr, maxx / dpr, maxy / dpr, int(colors.size()));
    // darkest/lightest
    QColor extreme = bg; int best = -1;
    for (QRgb c : colors) { int d = std::abs(qGray(c) - bg.lightness()); if (d > best) { best = d; extreme = QColor::fromRgba(c);} }
    std::printf("  most contrasting color=%s\n", qPrintable(extreme.name(QColor::HexArgb)));
    pm.save(out);
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    QApplication app(argc, argv);
    QString dir = argc > 1 ? argv[1] : ".";
    QString tag = QGuiApplication::platformName();
    std::printf("platform=%s screen dpr=%.1f\n", qPrintable(tag), app.devicePixelRatio());

    RectWidget w;
    w.move(0, 0);
    auto* effect = new hpq::HandPaintEffect(hpq::Theme::sketch());
    w.setGraphicsEffect(effect);
    w.show();
    QTest::qWait(200);
    std::printf("theme is sketch: %d\n", effect->theme() == hpq::Theme::sketch());
    report("B22 sketch", w.grab(), dir + "/B22_sketch_" + tag + ".png");
    effect->set_theme(hpq::Theme::chalk());
    std::printf("theme is chalk: %d\n", effect->theme() == hpq::Theme::chalk());
    report("B23 chalk", w.grab(), dir + "/B23_chalk_" + tag + ".png");
    QTest::qWait(100);
    report("B23 chalk after events", w.grab(), dir + "/B23_chalk_after_events_" + tag + ".png");

    // curiosity: frame with controls
    QFrame frame;
    auto* lay = new QVBoxLayout(&frame);
    lay->addWidget(new QLabel("Hello label"));
    lay->addWidget(new QPushButton("Push me"));
    lay->addWidget(new QCheckBox("Check me"));
    auto* le = new QLineEdit; le->setText("text"); lay->addWidget(le);
    frame.setGraphicsEffect(new hpq::HandPaintEffect(hpq::Theme::chalk()));
    frame.resize(220, 180);
    frame.show();
    QTest::qWait(200);
    report("frame chalk", frame.grab(), dir + "/extra_frame_chalk_" + tag + ".png");

    QMainWindow mw;
    auto* central = new QFrame;
    auto* l2 = new QVBoxLayout(central);
    l2->addWidget(new QPushButton("Central button"));
    l2->addWidget(new QCheckBox("Central check"));
    mw.setCentralWidget(central);
    central->setGraphicsEffect(new hpq::HandPaintEffect(hpq::Theme::sketch()));
    mw.resize(240, 140);
    mw.show();
    QTest::qWait(200);
    report("mainwindow sketch", mw.grab(), dir + "/extra_mainwindow_sketch_" + tag + ".png");
    return 0;
}
