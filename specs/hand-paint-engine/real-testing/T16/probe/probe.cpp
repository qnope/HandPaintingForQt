// T16 real-test probe: drives hpq::example::MainWindow like a user (QTest clicks/keys),
// records every Qt message with its type, and grabs PNGs.
#include "AppFont.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPixmap>
#include <QTest>
#include <cstdio>

static int g_warnings = 0;
static QtMessageHandler g_prev = nullptr;

static void handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (type != QtDebugMsg && type != QtInfoMsg)
        ++g_warnings;
    g_prev(type, ctx, msg); // default handler, honours QT_MESSAGE_PATTERN
}

int main(int argc, char *argv[])
{
    g_prev = qInstallMessageHandler(handler);
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    const QString out = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    const QString suffix = QGuiApplication::platformName();

    hpq::example::MainWindow window;
    window.show();
    std::printf("exposed=%d\n", int(QTest::qWaitForWindowExposed(&window)));
    QApplication::processEvents();
    const int warnings_after_show = g_warnings;
    std::printf("warnings after show (offscreen plugin notice expected) = %d\n", warnings_after_show);

    auto *combo = window.findChild<QComboBox *>("comboBox");
    auto *check = window.findChild<QCheckBox *>("checkBox");
    auto *line = window.findChild<QLineEdit *>("lineEdit");
    std::printf("found combo=%d check=%d line=%d\n", combo != nullptr, check != nullptr, line != nullptr);
    if (!combo || !check || !line)
        return 2;

    // ---- B28: each theme in turn then None, processing events after each.
    const int order[] = {1, 2, 3, 0};
    for (int idx : order) {
        combo->setCurrentIndex(idx);
        QApplication::processEvents();
        QTest::qWait(50);
        const QString name = combo->itemText(idx);
        window.grab().save(QString("%1/b28_%2_%3_%4.png").arg(out).arg(idx).arg(QString(name).replace(' ', '_')).arg(suffix));
        std::printf("B28 selected '%s' -> visible=%d, warnings since show=%d\n", qPrintable(name), window.isVisible(),
                    g_warnings - warnings_after_show);
    }

    // ---- B27: "Black slate and chalk" selected; click checkBox, type "abc".
    combo->setCurrentIndex(3);
    QApplication::processEvents();
    QTest::qWait(50);
    window.grab().save(QString("%1/b27_before_%2.png").arg(out, suffix));
    const bool before = check->isChecked();
    QTest::mouseClick(check, Qt::LeftButton);
    QTest::keyClicks(line, "abc");
    QApplication::processEvents();
    QTest::qWait(50);
    std::printf("B27 theme='%s' checkBox before=%d after=%d lineEdit='%s'\n", qPrintable(combo->currentText()), before,
                check->isChecked(), qPrintable(line->text()));
    window.grab().save(QString("%1/b27_after_%2.png").arg(out, suffix));
    // crop of the area around the checkbox and line edit for a closer look
    const QRect r = check->geometry().united(line->geometry()).adjusted(-30, -30, 30, 30);
    const QPoint tl = check->parentWidget()->mapTo(&window, r.topLeft());
    window.grab(QRect(tl, r.size())).scaled(r.size() * 2).save(QString("%1/b27_after_zoom_%2.png").arg(out, suffix));

    std::printf("total warnings since show=%d\n", g_warnings - warnings_after_show);
    return 0;
}
