// Nested layouts: widgets inside a group box inside a splitter / tab widget / scroll area.
// Grabs themed vs plain (Fusion + Sketch palette) windows and saves them.
#include <hpq/HandPaintEffect.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStyle>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <cstdio>

static int warnings = 0;
static void handler(QtMsgType t, const QMessageLogContext &, const QString &msg) {
    if (t != QtDebugMsg) ++warnings;
    std::fprintf(stderr, "[qt msg %d] %s\n", int(t), qPrintable(msg));
}

static QGroupBox *group(const QString &title, int n) {
    auto *g = new QGroupBox(title);
    auto *l = new QVBoxLayout(g);
    l->addWidget(new QLabel(QString("Label %1").arg(n)));
    auto *le = new QLineEdit(QString("Edit %1").arg(n));
    l->addWidget(le);
    auto *cb = new QComboBox;
    cb->addItems({QString("Combo %1").arg(n), "Other"});
    l->addWidget(cb);
    auto *sp = new QSpinBox;
    sp->setValue(40 + n);
    l->addWidget(sp);
    auto *c = new QCheckBox(QString("Check %1").arg(n));
    c->setChecked(true);
    l->addWidget(c);
    l->addWidget(new QRadioButton(QString("Radio %1").arg(n)));
    auto *s = new QSlider(Qt::Horizontal);
    s->setValue(60);
    l->addWidget(s);
    l->addWidget(new QPushButton(QString("Button %1").arg(n)));
    return g;
}

static void build(QMainWindow &w) {
    auto *split = new QSplitter(Qt::Horizontal);
    // 1. group box in a splitter
    split->addWidget(group("In splitter", 1));
    // 2. group box in a tab widget, plus nested group in group
    auto *tabs = new QTabWidget;
    auto *page = new QWidget;
    auto *pl = new QVBoxLayout(page);
    auto *outer = new QGroupBox("Outer group");
    auto *ol = new QVBoxLayout(outer);
    ol->addWidget(group("Inner in tab", 2));
    pl->addWidget(outer);
    tabs->addTab(page, "First tab");
    auto *p2 = new QWidget;
    auto *p2l = new QVBoxLayout(p2);
    auto *lw = new QListWidget;
    lw->addItems({"List one", "List two"});
    p2l->addWidget(lw);
    tabs->addTab(p2, "Second tab");
    split->addWidget(tabs);
    // 3. group box in a scroll area (scrolled by 30 px)
    auto *scroll = new QScrollArea;
    auto *inner = new QWidget;
    auto *il = new QVBoxLayout(inner);
    il->addWidget(group("In scroll A", 3));
    il->addWidget(group("In scroll B", 4));
    auto *te = new QTextEdit;
    te->setPlainText("Text edit line 1\nline 2");
    te->setMinimumHeight(80);
    il->addWidget(te);
    scroll->setWidget(inner);
    scroll->setWidgetResizable(true);
    split->addWidget(scroll);
    w.setCentralWidget(split);
    w.resize(820, 480);
}

int main(int argc, char **argv) {
    qInstallMessageHandler(handler);
    QApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";
    auto theme = std::make_shared<hpq::SketchTheme>();

    QMainWindow plain;
    build(plain);
    QStyle *fusion = QStyleFactory::create("Fusion");
    plain.setStyle(fusion);
    for (auto *c : plain.findChildren<QWidget *>()) c->setStyle(fusion);
    plain.setPalette(theme->palette());
    plain.show();
    app.processEvents();
    if (auto *sa = plain.findChild<QScrollArea *>()) sa->verticalScrollBar();
    QImage ip = plain.grab().toImage();
    ip.save(out + "/nested_plain.png");

    QMainWindow w;
    build(w);
    hpq::apply_theme(w, theme, std::make_shared<hpq::RoughGeometryEngine>());
    w.show();
    app.processEvents();
    QImage it = w.grab().toImage();
    it.save(out + "/nested_themed.png");

    // Scroll both by 60px and grab again
    for (QMainWindow *m : {&plain, &w}) {
        auto *sa = m->findChild<QScrollArea *>();
        sa->verticalScrollBar()->setValue(60);
        m->findChild<QTabWidget *>()->setCurrentIndex(0);
    }
    app.processEvents();
    plain.grab().save(out + "/nested_plain_scrolled.png");
    w.grab().save(out + "/nested_themed_scrolled.png");
    // Second tab
    plain.findChild<QTabWidget *>()->setCurrentIndex(1);
    w.findChild<QTabWidget *>()->setCurrentIndex(1);
    app.processEvents();
    plain.grab().save(out + "/nested_plain_tab2.png");
    w.grab().save(out + "/nested_themed_tab2.png");

    std::printf("themed differs from plain: %d\n", it != ip);
    std::printf("effect: %d\n", qobject_cast<hpq::HandPaintEffect *>(w.graphicsEffect()) != nullptr);
    std::printf("qt warnings: %d\n", warnings);
    return 0;
}
