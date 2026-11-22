#include "AppFont.h"
#include "MainWindow.h"
#include "PlatformMessages.h"

#include <QApplication>
#include <QTimer>

namespace {
void show_main_window(QWidget &window)
{
#ifdef Q_OS_WASM
    // In a browser the page is the window frame: the window fills it, without a title bar.
    window.showFullScreen();
#else
    window.show();
#endif
}
} // namespace

int main(int argc, char *argv[])
{
    const bool auto_quit = qEnvironmentVariableIsSet("HPQ_AUTO_QUIT");
    if (auto_quit)
        hpq::example::install_unexpected_warning_marker();

    QApplication app(argc, argv);
    hpq::example::apply_default_font();

    hpq::example::MainWindow window;
    show_main_window(window);

    // Used by the smoke test: quit as soon as the event loop starts.
    if (auto_quit)
        QTimer::singleShot(0, &app, &QApplication::quit);

    return app.exec();
}
