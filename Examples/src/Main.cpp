#include "AppFont.h"
#include "MainWindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    hpq::example::apply_default_font();

    hpq::example::MainWindow window;
    window.show();

    // Used by the smoke test: quit as soon as the event loop starts.
    if (qEnvironmentVariableIsSet("HPQ_AUTO_QUIT"))
        QTimer::singleShot(0, &app, &QApplication::quit);

    return app.exec();
}
