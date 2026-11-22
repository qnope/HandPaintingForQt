#include <hpq/SketchTheme.h>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QLabel>
#include <QPainter>
#include <QImage>
#include <QVBoxLayout>
#include <QWidget>
#include <cstdio>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const char *out = argc > 1 ? argv[1] : ".";
    std::printf("before theme: families contains AD = %d\n",
                QFontDatabase::families().contains("Architects Daughter"));
    hpq::SketchTheme theme;
    QFont f = theme.font();
    std::printf("theme.font().family() = '%s'\n", qPrintable(f.family()));
    std::printf("QFontInfo(font).family() = '%s'\n", qPrintable(QFontInfo(f).family()));
    std::printf("after theme: families contains AD = %d\n",
                QFontDatabase::families().contains("Architects Daughter"));
    const char *res[] = {":/hpq/fonts/ArchitectsDaughter-Regular.ttf", ":/hpq/fonts/OFL.txt"};
    for (auto r : res) {
        QFile file(r);
        std::printf("exists %s = %d size=%lld\n", r, file.exists(), (long long)file.size());
    }
    QFile ofl(":/hpq/fonts/OFL.txt");
    ofl.open(QIODevice::ReadOnly);
    std::printf("OFL head: %s\n", ofl.readLine(200).trimmed().constData());
    std::printf("OFL contains 'SIL OPEN FONT LICENSE' = %d\n",
                (int)ofl.readAll().contains("SIL OPEN FONT LICENSE"));

    // A window with labels: theme font vs. a fallback font for comparison.
    QWidget w;
    w.setStyleSheet("background:white");
    auto *lay = new QVBoxLayout(&w);
    QFont big = theme.font(); big.setPixelSize(32);
    auto *l1 = new QLabel("Architects Daughter: The quick brown fox 0123", &w);
    l1->setFont(big);
    QFont helv("Helvetica"); helv.setPixelSize(32);
    auto *l2 = new QLabel("Helvetica: The quick brown fox 0123", &w);
    l2->setFont(helv);
    QFont small = theme.font();
    auto *l3 = new QLabel("Default size theme font: Hello, hand painting!", &w);
    l3->setFont(small);
    lay->addWidget(l1); lay->addWidget(l2); lay->addWidget(l3);
    w.resize(900, 220);
    w.show();
    app.processEvents();
    QString path = QString(out) + "/label_grab.png";
    std::printf("grab saved=%d %s\n", w.grab().save(path), qPrintable(path));

    QImage img(900, 120, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setFont(big);
    p.drawText(20, 50, "QPainter + theme.font(): Sketchy text");
    p.setFont(helv);
    p.drawText(20, 100, "QPainter + Helvetica: Sketchy text");
    p.end();
    path = QString(out) + "/painter_text.png";
    std::printf("painter saved=%d\n", img.save(path));
    return 0;
}
