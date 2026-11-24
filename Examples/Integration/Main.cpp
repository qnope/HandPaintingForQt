#include "GalleryWindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    hpq::gallery::GalleryWindow window;
    window.show();

    // Used by the CI, which runs the application offscreen: quit as soon as the event loop starts.
    if (qEnvironmentVariableIsSet("HPQ_AUTO_QUIT")) {
        QTimer::singleShot(0, &application, &QApplication::quit);
    }
    return application.exec();
}
