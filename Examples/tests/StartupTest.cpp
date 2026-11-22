#include "MainWindow.h"
#include "PlatformMessages.h"

#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>
#include <QWindow>
#include <gtest/gtest.h>
#include <hpq/HandPaintStyle.h>

#include <string>
#include <vector>

namespace {
std::vector<std::string> collected_warnings;

void collect_warnings(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (hpq::example::is_unexpected_warning(type, context, message))
        collected_warnings.push_back(message.toStdString());
}

// Collects the warnings, not from the offscreen platform, sent while it lives.
class WarningCollector
{
public:
    WarningCollector() : m_previous_handler(qInstallMessageHandler(collect_warnings))
    {
        collected_warnings.clear();
    }
    ~WarningCollector() { qInstallMessageHandler(m_previous_handler); }
    WarningCollector(const WarningCollector &) = delete;
    WarningCollector &operator=(const WarningCollector &) = delete;

    const std::vector<std::string> &warnings() const { return collected_warnings; }

private:
    QtMessageHandler m_previous_handler;
};

// Processes events until the window of `widget` is exposed, or a time limit; true if exposed.
bool wait_for_window_exposed(QWidget &widget)
{
    constexpr int TimeoutMs = 5000;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < TimeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        const QWindow *window = widget.windowHandle();
        if (window != nullptr && window->isExposed())
            return true;
    }
    return false;
}
} // namespace

TEST(Startup, MainWindowShowsAndRunsEventLoopWithoutWarning)
{
    WarningCollector collector;
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
    EXPECT_EQ(std::vector<std::string>{}, collector.warnings());
}

TEST(Startup, ThemedMainWindowExposedPaintsWithoutWarning)
{
    WarningCollector collector;
    {
        hpq::example::MainWindow window;
        window.show();
        ASSERT_TRUE(wait_for_window_exposed(window));
        window.repaint();
        QCoreApplication::processEvents();
    }
    EXPECT_EQ(std::vector<std::string>{}, collector.warnings());
}

TEST(Startup, ThemedMainWindowGrabbedPaintsWithoutWarning)
{
    WarningCollector collector;
    {
        hpq::example::MainWindow window;
        ASSERT_NE(qobject_cast<hpq::HandPaintStyle *>(window.findChild<QComboBox *>("comboBox")->style()),
                  nullptr);
        const QSize window_size(800, 600);
        window.resize(window_size);
        const QPixmap grab = window.grab();
        EXPECT_FALSE(grab.isNull());
        EXPECT_EQ(grab.size() / grab.devicePixelRatio(), window_size);
    }
    EXPECT_EQ(std::vector<std::string>{}, collector.warnings());
}

TEST(Startup, OffscreenPlatformMessagesAreNotUnexpectedWarnings)
{
    const QMessageLogContext font_context(nullptr, 0, nullptr, "qt.qpa.fonts");
    EXPECT_FALSE(hpq::example::is_unexpected_warning(
        QtWarningMsg, font_context, "Populating font family aliases took 50 ms."));
    const QMessageLogContext default_context(nullptr, 0, nullptr, "default");
    EXPECT_FALSE(hpq::example::is_unexpected_warning(
        QtWarningMsg, default_context, "This plugin does not support propagateSizeHints()"));
}

TEST(Startup, OtherWarningsAreUnexpected)
{
    const QMessageLogContext default_context(nullptr, 0, nullptr, "default");
    EXPECT_TRUE(hpq::example::is_unexpected_warning(
        QtWarningMsg, default_context, "QPainter::begin: Paint device returned engine == 0"));
    EXPECT_TRUE(hpq::example::is_unexpected_warning(QtCriticalMsg, default_context, "critical"));
    EXPECT_FALSE(hpq::example::is_unexpected_warning(QtDebugMsg, default_context, "debug"));
}
