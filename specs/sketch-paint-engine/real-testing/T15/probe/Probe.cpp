#include "AppFont.h"
#include "MainWindow.h"
#include <hpq/HandPaintEffect.h>
#include <hpq/SketchTheme.h>
#include <QApplication>
#include <QComboBox>
#include <QScreen>
#include <QWindow>
#include <QPixmap>
#include <cstdio>

static void report(QMainWindow &w, const char *tag, const QString &dir)
{
    for (int i = 0; i < 5; ++i) QApplication::processEvents();
    auto *combo = w.findChild<QComboBox *>("comboBox");
    auto *eff = w.graphicsEffect();
    bool is_hpe = dynamic_cast<hpq::HandPaintEffect *>(eff) != nullptr;
    hpq::SketchTheme sketch;
    std::printf("[%s] combo index=%d text='%s' effect=%s(%s) palette==Sketch:%d font='%s' window=%s\n",
                tag, combo->currentIndex(), qPrintable(combo->currentText()),
                eff ? eff->metaObject()->className() : "null", is_hpe ? "HandPaintEffect" : "-",
                int(w.palette() == sketch.palette()), qPrintable(w.font().family()),
                qPrintable(w.palette().color(QPalette::Window).name()));
    QPixmap g = w.grab();
    g.save(dir + "/" + tag + "_grab.png");
    QPixmap s = w.windowHandle()->screen()->grabWindow(w.winId());
    s.save(dir + "/" + tag + "_screen.png");
    std::fflush(stdout);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    QString dir = argv[1];
    hpq::example::MainWindow w;
    w.show();
    report(w, "1_startup", dir);
    auto *combo = w.findChild<QComboBox *>("comboBox");
    combo->setCurrentIndex(1); report(w, "2_china_ink", dir);
    combo->setCurrentIndex(2); report(w, "3_black_slate", dir);
    combo->setCurrentIndex(0); report(w, "4_sketch_again", dir);
    combo->setCurrentIndex(1); combo->setCurrentIndex(0); report(w, "5_china_then_sketch", dir);
    return 0;
}
