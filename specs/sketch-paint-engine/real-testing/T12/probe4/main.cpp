// Group box nested in group box: directly in central widget, and inside a tab widget.
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
static QGroupBox *nested(const QString &outer) {
    auto *o = new QGroupBox(outer);
    auto *ol = new QVBoxLayout(o);
    auto *i = new QGroupBox("Inner title");
    auto *il = new QVBoxLayout(i);
    il->addWidget(new QLabel("Inner label"));
    ol->addWidget(i);
    return o;
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QMainWindow w;
    auto *c = new QWidget;
    auto *l = new QHBoxLayout(c);
    l->addWidget(nested("Outer direct"));
    auto *tabs = new QTabWidget;
    auto *page = new QWidget;
    (new QVBoxLayout(page))->addWidget(nested("Outer in tab"));
    tabs->addTab(page, "Tab");
    l->addWidget(tabs);
    auto *single = new QWidget;
    auto *sl = new QVBoxLayout(single);
    auto *t2 = new QTabWidget;
    auto *p2 = new QWidget;
    auto *g = new QGroupBox("Single in tab");
    (new QVBoxLayout(g))->addWidget(new QLabel("label"));
    (new QVBoxLayout(p2))->addWidget(g);
    t2->addTab(p2, "Tab2");
    sl->addWidget(t2);
    l->addWidget(single);
    w.setCentralWidget(c);
    w.resize(640, 220);
    hpq::apply_theme(w, std::make_shared<hpq::SketchTheme>(), std::make_shared<hpq::RoughGeometryEngine>());
    w.show();
    app.processEvents();
    w.grab().save(QString(argv[1]) + "/nested_groups_themed.png");
    return 0;
}
