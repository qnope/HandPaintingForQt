// Real-user probe for T13 (B23, B24): a themed QMainWindow with many widgets.
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>
#include <hpq/HandPaintEffect.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QTest>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <cstdio>
#include <memory>

static QFile *g_log = nullptr;
static int g_bad = 0;  // messages not from qt.qpa.* and not the propagateSizeHints notice

static void handler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    const char *kinds[] = {"DEBUG", "WARNING", "CRITICAL", "FATAL", "INFO"};
    QString cat = ctx.category ? QString::fromLatin1(ctx.category) : QString();
    bool ignored = cat.startsWith("qt.qpa") || msg.contains("propagateSizeHints");
    QString line = QString("[%1][%2]%3 %4\n").arg(kinds[type], cat, ignored ? "[ignored]" : "", msg);
    if (g_log) { g_log->write(line.toUtf8()); g_log->flush(); }
    fprintf(stderr, "%s", line.toUtf8().constData());
    if (!ignored && type != QtDebugMsg && type != QtInfoMsg) ++g_bad;
}

int main(int argc, char **argv) {
    QString out = argc > 1 ? argv[1] : ".";
    QFile log(out + "/messages.log");
    log.open(QIODevice::WriteOnly | QIODevice::Truncate);
    g_log = &log;
    qInstallMessageHandler(handler);

    QElapsedTimer timer; timer.start();
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("hpq T13 probe");
    auto *tabs = new QTabWidget;
    // Tab 1: form widgets
    auto *page1 = new QWidget; auto *l1 = new QVBoxLayout(page1);
    auto *button = new QPushButton("Click me");
    auto *check = new QCheckBox("Check me");
    auto *edit = new QLineEdit; edit->setPlaceholderText("type here");
    auto *combo = new QComboBox; combo->addItems({"Apple", "Banana", "Cherry"});
    auto *label = new QLabel("A label with some text");
    l1->addWidget(label); l1->addWidget(button); l1->addWidget(check);
    l1->addWidget(edit); l1->addWidget(combo);
    auto *table = new QTableView;
    auto *model = new QStandardItemModel(6, 3, &window);
    for (int r = 0; r < 6; ++r) for (int c = 0; c < 3; ++c)
        model->setItem(r, c, new QStandardItem(QString("r%1c%2").arg(r).arg(c)));
    table->setModel(model);
    l1->addWidget(table);
    tabs->addTab(page1, "Form");
    // Tab 2: tree + graphics view
    auto *page2 = new QWidget; auto *l2 = new QVBoxLayout(page2);
    auto *tree = new QTreeWidget; tree->setHeaderLabels({"Name", "Value"});
    for (int i = 0; i < 4; ++i) {
        auto *it = new QTreeWidgetItem(tree, {QString("Node %1").arg(i), QString::number(i)});
        new QTreeWidgetItem(it, {"child", "x"});
    }
    tree->expandAll();
    auto *scene = new QGraphicsScene(&window);
    scene->addEllipse(0, 0, 120, 80, QPen(Qt::black), QBrush(Qt::yellow));
    scene->addRect(140, 10, 80, 60);
    scene->addText("Graphics");
    auto *gview = new QGraphicsView(scene);
    l2->addWidget(tree); l2->addWidget(gview);
    tabs->addTab(page2, "Tree/Graphics");
    // Tab 3: scroll area
    auto *scroll = new QScrollArea;
    auto *big = new QWidget; auto *lb = new QVBoxLayout(big);
    for (int i = 0; i < 40; ++i) lb->addWidget(new QPushButton(QString("Row button %1").arg(i)));
    scroll->setWidget(big);
    tabs->addTab(scroll, "Scroll");
    window.setCentralWidget(tabs);
    window.resize(640, 560);

    if (!qEnvironmentVariableIsSet("PROBE_NO_THEME"))
    hpq::apply_theme(window, std::make_shared<hpq::SketchTheme>(),
                     std::make_shared<hpq::RoughGeometryEngine>());
    QTextStream(stdout) << "graphicsEffect is HandPaintEffect: "
                        << (qobject_cast<hpq::HandPaintEffect *>(window.graphicsEffect()) != nullptr) << "\n";

    int clicked = 0;
    QObject::connect(button, &QPushButton::clicked, [&] { ++clicked; });

    window.show();
    bool exposed = QTest::qWaitForWindowExposed(&window);
    QTextStream(stdout) << "exposed: " << exposed << "\n";
    for (int i = 0; i < 5; ++i) { window.repaint(); QTest::qWait(20); }
    window.grab().save(out + "/01_initial.png");

    // Hover / mouse moves
    for (QWidget *w : {static_cast<QWidget *>(button), static_cast<QWidget *>(check),
                       static_cast<QWidget *>(edit), static_cast<QWidget *>(table)}) {
        QTest::mouseMove(w); QTest::qWait(20);
    }
    // B24: click the checkbox
    bool before = check->isChecked();
    QTest::mouseClick(check, Qt::LeftButton);
    QTest::qWait(30);
    bool after_center = check->isChecked();
    QTextStream(stdout) << "checkbox width: " << check->width() << " center click toggled: " << after_center << "\n";
    if (!after_center) { QTest::mouseClick(check, Qt::LeftButton, {}, QPoint(8, check->height() / 2)); QTest::qWait(30); }
    bool after = check->isChecked();
    QTest::mouseClick(button, Qt::LeftButton);
    QTest::qWait(30);
    // Type in the line edit
    QTest::mouseClick(edit, Qt::LeftButton);
    QTest::keyClicks(edit, "hello hpq");
    QTest::qWait(30);
    combo->setCurrentIndex(2);
    table->selectRow(2);
    window.repaint();
    window.grab().save(out + "/02_after_input.png");
    QTextStream(stdout) << "checkbox before: " << before << " after: " << after << "\n"
                        << "button clicked count: " << clicked << "\n"
                        << "line edit text: '" << edit->text() << "'\n";
    // Second click unchecks
    QTest::mouseClick(check, Qt::LeftButton);
    QTextStream(stdout) << "checkbox after 2nd click: " << check->isChecked() << "\n";
    QTest::mouseClick(check, Qt::LeftButton);

    // Resize
    window.resize(820, 640); QTest::qWait(50); window.repaint();
    window.grab().save(out + "/03_resized.png");
    window.resize(480, 420); QTest::qWait(50); window.repaint();

    // Switch tabs
    tabs->setCurrentIndex(1); QTest::qWait(50); window.repaint();
    window.grab().save(out + "/04_tab_tree_graphics.png");
    tabs->setCurrentIndex(2); QTest::qWait(50);
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum() / 2);
    QTest::qWait(30);
    QWheelEvent *dummy = nullptr; (void)dummy;
    window.repaint();
    window.grab().save(out + "/05_tab_scroll.png");
    QTextStream(stdout) << "scroll value: " << scroll->verticalScrollBar()->value() << "\n";
    tabs->setCurrentIndex(0);

    // Event loop for a while with repeated repaints
    for (int i = 0; i < 50; ++i) { window.update(); QTest::qWait(10); }
    window.grab().save(out + "/06_final.png");

    QTextStream(stdout) << "elapsed ms: " << timer.elapsed() << "\n"
                        << "non-ignored warnings: " << g_bad << "\n";
    return g_bad == 0 && after && !before && clicked == 1 && edit->text() == "hello hpq" ? 0 : 1;
}
