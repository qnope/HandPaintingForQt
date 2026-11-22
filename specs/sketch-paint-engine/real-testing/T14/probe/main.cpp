#include <hpq/Theming.h>
#include <hpq/SketchTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/HandPaintEffect.h>
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QStyleFactory>
#include <QImage>
#include <QPixmap>
#include <cstdio>

struct Snap { QWidget *w; QPalette pal; QFont font; QString style; bool own_style; bool pal_attr; bool font_attr; };
static QList<Snap> snap(QWidget &root) {
    QList<Snap> out; QList<QWidget*> ws{&root}; ws += root.findChildren<QWidget*>();
    for (auto *w : ws) out.push_back({w, w->palette(), w->font(), w->style()->name(),
        w->testAttribute(Qt::WA_SetStyle), w->testAttribute(Qt::WA_SetPalette), w->testAttribute(Qt::WA_SetFont)});
    return out;
}
static int failures = 0;
static void check(const char *step, QWidget &root, const QList<Snap> &before) {
    int bad = 0;
    if (root.graphicsEffect()) { printf("  [%s] FAIL window still has effect\n", step); ++bad; }
    int effects = root.findChildren<hpq::HandPaintEffect*>().size();
    if (effects) { printf("  [%s] FAIL %d HandPaintEffect children remain\n", step, effects); ++bad; }
    auto now = snap(root);
    for (int i = 0; i < before.size(); ++i) {
        const auto &a = before[i]; const auto &b = now[i];
        QString n = a.w->objectName();
        if (a.pal != b.pal) { printf("  [%s] FAIL palette differs on %s\n", step, qPrintable(n)); ++bad; }
        if (a.font != b.font) { printf("  [%s] FAIL font differs on %s (%s vs %s)\n", step, qPrintable(n), qPrintable(a.font.toString()), qPrintable(b.font.toString())); ++bad; }
        if (a.style != b.style) { printf("  [%s] FAIL style differs on %s (%s vs %s)\n", step, qPrintable(n), qPrintable(a.style), qPrintable(b.style)); ++bad; }
        if (a.own_style != b.own_style || a.pal_attr != b.pal_attr || a.font_attr != b.font_attr)
            printf("  [%s] NOTE attribute flags differ on %s (style %d->%d pal %d->%d font %d->%d)\n", step, qPrintable(n), a.own_style,b.own_style,a.pal_attr,b.pal_attr,a.font_attr,b.font_attr);
    }
    printf("  [%s] %s (%d widgets checked)\n", step, bad ? "FAILED" : "OK", (int)before.size());
    failures += bad;
}
static QImage grab(QWidget &w, const QString &file) {
    QApplication::processEvents();
    QImage img = w.grab().toImage();
    img.save(file);
    return img;
}
static void compare(const char *name, const QImage &a, const QImage &b) {
    if (a.size() != b.size()) { printf("  [%s] images differ in size\n", name); ++failures; return; }
    int diff = 0;
    for (int y = 0; y < a.height(); ++y) for (int x = 0; x < a.width(); ++x) if (a.pixel(x,y) != b.pixel(x,y)) ++diff;
    printf("  [%s] differing pixels: %d / %d\n", name, diff, a.width()*a.height());
    if (diff) ++failures;
}

static QMainWindow *make_window(const QString &tag) {
    auto *win = new QMainWindow; win->setObjectName("window_" + tag);
    win->setWindowTitle("T14 probe"); win->resize(420, 300);
    auto *central = new QWidget; central->setObjectName("central");
    auto *lay = new QVBoxLayout(central);
    auto *label = new QLabel("Default label"); label->setObjectName("label_default");
    auto *button = new QPushButton("Default button"); button->setObjectName("button_default");
    auto *red = new QPushButton("Red palette button"); red->setObjectName("button_red");
    QPalette p = red->palette(); p.setColor(QPalette::Button, Qt::red); p.setColor(QPalette::ButtonText, Qt::white); red->setPalette(p);
    auto *big = new QLabel("Courier 18 bold label"); big->setObjectName("label_courier");
    QFont f("Courier New", 18); f.setBold(true); big->setFont(f);
    auto *win_style = new QCheckBox("Check box with Windows style"); win_style->setObjectName("check_windows");
    win_style->setStyle(QStyleFactory::create("Windows")); win_style->setChecked(true);
    auto *edit = new QLineEdit("line edit text"); edit->setObjectName("edit_default");
    auto *combo = new QComboBox; combo->addItems({"One","Two"}); combo->setObjectName("combo_default");
    for (auto *w : std::initializer_list<QWidget*>{label, button, red, big, win_style, edit, combo}) lay->addWidget(w);
    win->setCentralWidget(central);
    return win;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const QString out = argc > 1 ? argv[1] : ".";
    auto theme = std::make_shared<hpq::SketchTheme>();
    auto engine = std::make_shared<hpq::RoughGeometryEngine>();

    printf("Scenario 1: apply then remove\n");
    QMainWindow *w = make_window("1"); w->show();
    auto s0 = snap(*w);
    QImage plain = grab(*w, out + "/01_plain_before.png");
    hpq::apply_theme(*w, theme, engine);
    printf("  after apply: effect=%s\n", w->graphicsEffect() ? w->graphicsEffect()->metaObject()->className() : "null");
    grab(*w, out + "/02_themed.png");
    hpq::remove_theme(*w);
    QImage after = grab(*w, out + "/03_plain_after_remove.png");
    check("apply/remove", *w, s0);
    compare("plain before vs after remove", plain, after);

    printf("Scenario 2: apply twice then remove\n");
    QMainWindow *w2 = make_window("2"); w2->show();
    auto s2 = snap(*w2);
    QImage plain2 = grab(*w2, out + "/04_plain_before_double.png");
    hpq::apply_theme(*w2, theme, engine);
    hpq::apply_theme(*w2, theme, engine);
    printf("  after 2x apply: effect=%s, HandPaintEffect children=%d\n",
        w2->graphicsEffect() ? w2->graphicsEffect()->metaObject()->className() : "null",
        (int)w2->findChildren<hpq::HandPaintEffect*>().size());
    grab(*w2, out + "/05_themed_twice.png");
    hpq::remove_theme(*w2);
    QImage after2 = grab(*w2, out + "/06_plain_after_double_remove.png");
    check("2x apply/remove", *w2, s2);
    compare("plain before vs after 2x apply/remove", plain2, after2);

    printf("Scenario 3: apply/remove 5 times\n");
    QMainWindow *w3 = make_window("3"); w3->show();
    auto s3 = snap(*w3);
    QImage plain3 = grab(*w3, out + "/07_plain_before_cycles.png");
    for (int i = 0; i < 5; ++i) {
        hpq::apply_theme(*w3, theme, engine);
        if (i == 4) grab(*w3, out + "/08_themed_cycle5.png");
        hpq::remove_theme(*w3);
        check(qPrintable(QString("cycle %1").arg(i+1)), *w3, s3);
    }
    QImage after3 = grab(*w3, out + "/09_plain_after_cycles.png");
    compare("plain before vs after 5 cycles", plain3, after3);

    printf("Scenario 4: remove_theme on never-themed window (also called twice after remove)\n");
    QMainWindow *w4 = make_window("4"); w4->show();
    auto s4 = snap(*w4);
    QImage plain4 = grab(*w4, out + "/10_never_themed_before.png");
    hpq::remove_theme(*w4);
    QImage after4 = grab(*w4, out + "/11_never_themed_after_remove.png");
    check("never themed", *w4, s4);
    compare("never themed", plain4, after4);
    hpq::remove_theme(*w); // second remove on already-restored window
    check("second remove on w1", *w, s0);

    printf("Scenario 5: apply after remove re-themes\n");
    hpq::apply_theme(*w, theme, engine);
    printf("  re-apply effect=%s, window font family=%s\n", w->graphicsEffect() ? "set" : "null", qPrintable(w->font().family()));
    grab(*w, out + "/12_rethemed.png");
    hpq::remove_theme(*w);
    check("re-apply/remove", *w, s0);

    delete w; delete w2; delete w3; delete w4;
    printf("TOTAL failures: %d\n", failures);
    return failures ? 1 : 0;
}
