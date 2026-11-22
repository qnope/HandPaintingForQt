// Real-user probe for T14/B24: open the example main window, report the comboBox
// content and the graphics effects, save screenshots of the window and of the open drop-down.
#include "AppFont.h"
#include "MainWindow.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <cstdio>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    const QString out = argc > 1 ? argv[1] : ".";
    hpq::example::MainWindow w;
    w.show();
    QTimer::singleShot(300, [&] {
        auto *combo = w.findChild<QComboBox *>("comboBox");
        if (!combo) { std::printf("comboBox NOT FOUND\n"); app.exit(2); return; }
        std::printf("count=%d\n", combo->count());
        for (int i = 0; i < combo->count(); ++i)
            std::printf("item[%d]=\"%s\"\n", i, qPrintable(combo->itemText(i)));
        std::printf("currentIndex=%d currentText=\"%s\"\n", combo->currentIndex(), qPrintable(combo->currentText()));
        std::printf("menuBar effect=%p\ncentralWidget effect=%p\nstatusBar effect=%p\n",
                    (void *)w.menuBar()->graphicsEffect(), (void *)w.centralWidget()->graphicsEffect(),
                    (void *)w.statusBar()->graphicsEffect());
        std::printf("window title=\"%s\" size=%dx%d\n", qPrintable(w.windowTitle()), w.width(), w.height());
        w.grab().save(out + "/window.png");
        combo->showPopup();
        QTimer::singleShot(300, [&, combo] {
            QWidget *popup = combo->view()->window();
            popup->grab().save(out + "/combo-popup.png");
            std::printf("popup visible=%d\n", popup->isVisible());
            combo->hidePopup();
            app.quit();
        });
    });
    return app.exec();
}
