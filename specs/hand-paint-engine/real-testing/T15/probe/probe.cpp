// Real-user probe for T15 (B25/B26): show the example main window, select each theme in the
// comboBox, grab the window after each selection, report the effects and a few pixels.
#include "AppFont.h"
#include "MainWindow.h"
#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>
#include <QApplication>
#include <QComboBox>
#include <QGraphicsView>
#include <QGroupBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <cstdio>

static const char *name_of(const hpq::Theme &t)
{
    if (t == hpq::Theme::sketch()) return "sketch";
    if (t == hpq::Theme::china_ink()) return "china_ink";
    if (t == hpq::Theme::chalk()) return "chalk";
    return "OTHER";
}

static void report(QWidget *w, const char *label)
{
    QGraphicsEffect *e = w->graphicsEffect();
    auto *h = dynamic_cast<hpq::HandPaintEffect *>(e);
    std::printf("  %-14s effect=%s theme=%s\n", label, e ? (h ? "HandPaintEffect" : "OTHER") : "none",
                h ? name_of(h->theme()) : "-");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    const QString out = argc > 1 ? argv[1] : ".";
    const QString tag = QGuiApplication::platformName();
    hpq::example::MainWindow w;
    w.show();
    QTimer::singleShot(500, [&] {
        auto *combo = w.findChild<QComboBox *>("comboBox");
        auto *group = w.findChild<QGroupBox *>("groupBox");
        // An empty area of the central widget: just right of the group box, in the layout margin, and
        // the gap between the group box and the views.
        QPoint empty1 = w.centralWidget()->mapTo(&w, QPoint(3, 3));
        QPoint empty2 = w.centralWidget()->mapTo(&w, QPoint(w.centralWidget()->width() - 3, group->geometry().bottom() + 3));
        std::printf("platform=%s window=%dx%d dpr=%.1f empty1=(%d,%d) empty2=(%d,%d)\n", qPrintable(tag), w.width(),
                    w.height(), w.devicePixelRatioF(), empty1.x(), empty1.y(), empty2.x(), empty2.y());
        auto pix = [&](const QImage &img, QPoint p) {
            const qreal d = img.devicePixelRatio();
            return QColor(img.pixel(int(p.x() * d), int(p.y() * d))).name(QColor::HexRgb).toUpper();
        };
        const QImage start = w.grab().toImage();
        start.save(out + "/0_start_" + tag + ".png");
        std::printf("start: empty1=%s empty2=%s\n", qPrintable(pix(start, empty1)), qPrintable(pix(start, empty2)));
        report(w.menuBar(), "menuBar");
        report(w.centralWidget(), "centralWidget");
        report(w.statusBar(), "statusBar");
        const char *files[] = {"", "1_sketch", "2_china_ink", "3_chalk"};
        for (int i : {1, 2, 3, 0}) {
            combo->setCurrentIndex(i);
            QApplication::processEvents();
            const QImage img = w.grab().toImage();
            const QString f = i == 0 ? QString("4_none") : QString(files[i]);
            img.save(out + "/" + f + "_" + tag + ".png");
            std::printf("selected \"%s\": empty1=%s empty2=%s\n", qPrintable(combo->currentText()),
                        qPrintable(pix(img, empty1)), qPrintable(pix(img, empty2)));
            report(w.menuBar(), "menuBar");
            report(w.centralWidget(), "centralWidget");
            report(w.statusBar(), "statusBar");
            if (i == 0)
                std::printf("grab after None == grab at start: %s\n", img == start ? "YES" : "NO");
        }
        // Second round: None after chalk directly too.
        combo->setCurrentIndex(3);
        QApplication::processEvents();
        combo->setCurrentIndex(0);
        QApplication::processEvents();
        std::printf("chalk->None grab == start: %s\n", w.grab().toImage() == start ? "YES" : "NO");
        app.quit();
    });
    return app.exec();
}
