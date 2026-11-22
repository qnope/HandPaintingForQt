#include "MainWindow.h"

#include <QApplication>
#include <QTimer>
#include <gtest/gtest.h>

namespace {
int warning_count = 0;

void counting_handler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    // The offscreen platform plugin does not implement propagateSizeHints() and warns for any
    // top-level widget with a layout (reproduced with a bare QWidget); not caused by the application.
    if (message.contains("propagateSizeHints"))
        return;
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        ++warning_count;
}
} // namespace

TEST(Startup, MainWindowShowsAndRunsEventLoopWithoutWarning)
{
    warning_count = 0;
    const QtMessageHandler previous = qInstallMessageHandler(counting_handler);
    {
        hpq::example::MainWindow window;
        window.show();
        bool visible_in_event_loop = false;
        QTimer::singleShot(0, qApp, [&] {
            visible_in_event_loop = window.isVisible();
            QApplication::quit();
        });
        qApp->exec();
        EXPECT_TRUE(visible_in_event_loop);
    }
    qInstallMessageHandler(previous);
    EXPECT_EQ(0, warning_count);
}
