#include <hpq/HandPaintEffect.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theming.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>
#include <QImage>
#include <cstdio>

static int warnings = 0;
static void handler(QtMsgType t, const QMessageLogContext &, const QString &msg) {
    if (t != QtDebugMsg) ++warnings;
    std::fprintf(stderr, "[qt msg %d] %s\n", int(t), qPrintable(msg));
}

static void build(QMainWindow &w) {
    w.setWindowTitle("Probe");
    auto *mb = w.menuBar();
    mb->addMenu("&File")->addAction("Open");
    mb->addMenu("&Edit");
    mb->addMenu("&Help");
    w.statusBar()->showMessage("Ready - status bar text");

    auto *central = new QWidget;
    auto *root = new QHBoxLayout(central);
    auto *left = new QVBoxLayout;
    root->addLayout(left);

    left->addWidget(new QLabel("Hello, hand painted world"));
    left->addWidget(new QPushButton("Push me"));
    auto *cb = new QCheckBox("Check me");
    cb->setChecked(true);
    left->addWidget(cb);
    auto *le = new QLineEdit;
    le->setText("Line edit text");
    left->addWidget(le);
    auto *combo = new QComboBox;
    combo->addItems({"Sketch", "China Ink", "Black slate"});
    left->addWidget(combo);
    auto *group = new QGroupBox("Group box");
    auto *gl = new QVBoxLayout(group);
    gl->addWidget(new QCheckBox("Inside group"));
    gl->addWidget(new QPushButton("Group button"));
    left->addWidget(group);
    auto *frame = new QFrame;
    frame->setFrameStyle(QFrame::Box | QFrame::Plain);
    frame->setLineWidth(2);
    frame->setMinimumSize(120, 50);
    auto *fl = new QVBoxLayout(frame);
    fl->addWidget(new QLabel("In a box frame"));
    left->addWidget(frame);

    auto *right = new QVBoxLayout;
    root->addLayout(right);
    auto *model = new QStandardItemModel(3, 2, &w);
    model->setHorizontalHeaderLabels({"Name", "Value"});
    const char *names[] = {"alpha", "beta", "gamma"};
    for (int r = 0; r < 3; ++r) {
        model->setItem(r, 0, new QStandardItem(names[r]));
        model->setItem(r, 1, new QStandardItem(QString::number(r * 10)));
    }
    auto *table = new QTableView;
    table->setModel(model);
    right->addWidget(table);

    auto *tmodel = new QStandardItemModel(&w);
    tmodel->setHorizontalHeaderLabels({"Tree"});
    auto *parent = new QStandardItem("Parent");
    parent->appendRow(new QStandardItem("Child 1"));
    parent->appendRow(new QStandardItem("Child 2"));
    tmodel->appendRow(parent);
    tmodel->appendRow(new QStandardItem("Sibling"));
    auto *tree = new QTreeView;
    tree->setModel(tmodel);
    tree->expandAll();
    right->addWidget(tree);

    auto *scene = new QGraphicsScene(&w);
    scene->addRect(0, 0, 80, 50, QPen(Qt::blue, 2), QBrush(Qt::yellow));
    scene->addEllipse(100, 0, 60, 60, QPen(Qt::red, 2), QBrush(Qt::green));
    scene->addText("Scene text");
    auto *view = new QGraphicsView(scene);
    right->addWidget(view);

    w.setCentralWidget(central);
    w.resize(720, 560);
}

int main(int argc, char **argv) {
    qInstallMessageHandler(handler);
    QApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";

    auto theme = std::make_shared<hpq::SketchTheme>();

    // Reference: same style + palette, no effect.
    QMainWindow plain;
    build(plain);
    QStyle *fusion = QStyleFactory::create("Fusion");
    plain.setStyle(fusion);
    for (auto *c : plain.findChildren<QWidget *>()) c->setStyle(fusion);
    plain.setPalette(theme->palette());
    plain.show();
    app.processEvents();
    QImage img_plain = plain.grab().toImage();
    img_plain.save(out + "/plain_fusion_sketchpalette.png");

    // Untouched default
    QMainWindow def;
    build(def);
    def.show();
    app.processEvents();
    def.grab().save(out + "/default.png");

    // Themed
    QMainWindow w;
    build(w);
    hpq::apply_theme(w, theme, std::make_shared<hpq::RoughGeometryEngine>());
    w.show();
    app.processEvents();
    QImage img_themed = w.grab().toImage();
    img_themed.save(out + "/themed.png");

    // B21 checks
    bool pal = w.palette() == theme->palette();
    std::printf("palette equal: %d\n", pal);
    std::printf("font family: %s (theme: %s)\n", qPrintable(w.font().family()),
                qPrintable(theme->font().family()));
    int total = 0, fusion_n = 0;
    std::printf("window style: %s\n", qPrintable(w.style()->name()));
    for (auto *c : w.findChildren<QWidget *>()) {
        ++total;
        if (c->style()->name() == "fusion") ++fusion_n;
        else std::printf("  non-fusion: %s %s\n", c->metaObject()->className(), qPrintable(c->style()->name()));
    }
    std::printf("descendants fusion: %d/%d\n", fusion_n, total);
    std::printf("effect is HandPaintEffect: %d\n",
                qobject_cast<hpq::HandPaintEffect *>(w.graphicsEffect()) != nullptr);
    // child font check
    auto *lbl = w.findChild<QLabel *>();
    std::printf("label font family: %s\n", qPrintable(lbl->font().family()));

    // B22
    std::printf("themed differs from plain: %d\n", img_themed != img_plain);
    // second grab for stability
    QImage again = w.grab().toImage();
    std::printf("themed grab stable across grabs: %d\n", again == img_themed);
    std::printf("sizes plain %dx%d themed %dx%d\n", img_plain.width(), img_plain.height(),
                img_themed.width(), img_themed.height());
    std::printf("qt warnings: %d\n", warnings);
    return 0;
}
