// Probe: is the menu bar text drawn when the menu bar has a HandPaintEffect?
#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QStyle>
#include <cstdio>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";
    std::printf("style=%s\n", qPrintable(QApplication::style()->name()));
    QMainWindow w;
    w.menuBar()->addMenu("File");
    w.menuBar()->addMenu("Edit");
    w.setCentralWidget(new QLabel("central"));
    w.statusBar()->showMessage("Ready");
    w.resize(300, 120);
    w.show();
    QTimer::singleShot(300, [&] {
        std::printf("menubar geom=%d,%d %dx%d nativeMenuBar=%d\n", w.menuBar()->x(), w.menuBar()->y(), w.menuBar()->width(),
                    w.menuBar()->height(), w.menuBar()->isNativeMenuBar());
        w.grab().save(out + "/mb_plain.png");
        w.menuBar()->setGraphicsEffect(new hpq::HandPaintEffect(hpq::Theme::sketch()));
        w.statusBar()->setGraphicsEffect(new hpq::HandPaintEffect(hpq::Theme::sketch()));
        QApplication::processEvents();
        w.grab().save(out + "/mb_sketch.png");
        app.quit();
    });
    return app.exec();
}
