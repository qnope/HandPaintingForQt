// Probe: shows the example MainWindow offscreen as main() does, records every Qt message,
// grabs the window to PNGs (Sketch theme, then "China Ink" for comparison).
#include "AppFont.h"
#include "MainWindow.h"
#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QPixmap>
#include <QScreen>
#include <QTextStream>
#include <QWindow>
#include <cstdio>

static QStringList messages;
static void handler(QtMsgType t, const QMessageLogContext &c, const QString &m)
{
    static const char *n[] = {"debug", "warning", "critical", "fatal", "info"};
    messages << QString("[%1] [%2] %3").arg(n[t]).arg(c.category ? c.category : "").arg(m);
}
static void spin(int ms) { QElapsedTimer t; t.start(); while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20); }

int main(int argc, char **argv)
{
    qInstallMessageHandler(handler);
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    hpq::example::MainWindow w;
    w.show();
    spin(800);
    std::printf("visible=%d exposed=%d size=%dx%d\n", w.isVisible(), w.windowHandle() && w.windowHandle()->isExposed(), w.width(), w.height());
    w.grab().save("mainwindow_sketch.png");
    QGuiApplication::primaryScreen()->grabWindow(w.winId()).save("screen_grab_sketch.png");
    auto *combo = w.findChild<QComboBox *>("comboBox");
    if (combo) { combo->setCurrentIndex(1); spin(300); w.grab().save("mainwindow_china_ink_untheme.png");
                 combo->setCurrentIndex(0); spin(300); w.grab().save("mainwindow_sketch_again.png"); }
    QFile f("probe_messages.txt"); f.open(QIODevice::WriteOnly);
    QTextStream(&f) << messages.join("\n") << "\n";
    std::printf("messages=%lld\n%s\n", (long long)messages.size(), qPrintable(messages.join("\n")));
    return 0;
}
