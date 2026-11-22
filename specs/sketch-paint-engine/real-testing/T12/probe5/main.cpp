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

// Localize the white streak over the splitter group box seen on the second tab.
int main(int argc, char **argv) {
    qInstallMessageHandler(handler);
    QApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";
    auto theme = std::make_shared<hpq::SketchTheme>();
    QMainWindow w;
    build(w);
    hpq::apply_theme(w, theme, std::make_shared<hpq::RoughGeometryEngine>());
    w.findChild<QTabWidget *>()->setCurrentIndex(1);
    w.show();
    app.processEvents();
    w.grab().save(out + "/p5_tab2_direct.png");
    w.findChild<QListWidget *>()->hide();
    app.processEvents();
    w.grab().save(out + "/p5_tab2_nolist.png");
    w.findChild<QListWidget *>()->show();
    w.findChild<QTabWidget *>()->hide();
    app.processEvents();
    w.grab().save(out + "/p5_notabs.png");
    return 0;
}
