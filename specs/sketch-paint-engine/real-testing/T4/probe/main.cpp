#include <hpq/SketchTheme.h>
#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaEnum>
#include <QPushButton>
#include <QStyleFactory>
#include <QWidget>
#include <cmath>
#include <cstdio>
#include <memory>

static double lin(double c) { c /= 255.0; return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
static double lum(const QColor &c) { return 0.2126 * lin(c.red()) + 0.7152 * lin(c.green()) + 0.0722 * lin(c.blue()); }
static double ratio(const QColor &a, const QColor &b) {
    double la = lum(a), lb = lum(b); if (la < lb) std::swap(la, lb); return (la + 0.05) / (lb + 0.05);
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    std::unique_ptr<hpq::Theme> theme = std::make_unique<hpq::SketchTheme>();
    QPalette pal = theme->palette();
    QMetaEnum roles = QMetaEnum::fromType<QPalette::ColorRole>();
    const char *gnames[] = {"Active", "Disabled", "Inactive"};
    QPalette::ColorGroup groups[] = {QPalette::Active, QPalette::Disabled, QPalette::Inactive};
    int unset = 0;
    for (int g = 0; g < 3; ++g) {
        std::printf("== %s ==\n", gnames[g]);
        for (int r = 0; r < QPalette::NColorRoles; ++r) {
            auto role = QPalette::ColorRole(r);
            bool set = pal.isBrushSet(groups[g], role);
            if (!set) ++unset;
            QColor c = pal.color(groups[g], role);
            std::printf("  %2d %-16s set=%d %s alpha=%d lightness=%d\n", r, roles.valueToKey(r),
                        set, qPrintable(c.name()), c.alpha(), c.lightness());
        }
    }
    std::printf("unset brushes: %d\n", unset);
    for (int g = 0; g < 3; ++g)
        std::printf("contrast WindowText/Window %s: %.2f  Text/Base: %.2f  ButtonText/Button: %.2f\n", gnames[g],
                    ratio(pal.color(groups[g], QPalette::WindowText), pal.color(groups[g], QPalette::Window)),
                    ratio(pal.color(groups[g], QPalette::Text), pal.color(groups[g], QPalette::Base)),
                    ratio(pal.color(groups[g], QPalette::ButtonText), pal.color(groups[g], QPalette::Button)));
    std::printf("fill_style Solid: %d\n", theme->fill_style() == hpq::FillStyle::Solid);
    auto o = theme->rough_options();
    std::printf("rough_options: roughness=%.2f stroke_count=%d max_offset=%.2f\n", o.roughness, o.stroke_count, o.max_offset);
    std::printf("font family: %s\n", qPrintable(theme->font().family()));

    QStyle *fusion = QStyleFactory::create("Fusion");
    QApplication::setStyle(fusion);
    QWidget w; w.setPalette(pal); w.setAutoFillBackground(true);
    auto *grid = new QGridLayout(&w);
    for (int col = 0; col < 2; ++col) {
        bool en = col == 0;
        grid->addWidget(new QLabel(en ? "Enabled" : "Disabled"), 0, col);
        auto *l = new QLabel("A graphite label"); grid->addWidget(l, 1, col);
        auto *b = new QPushButton("Push me"); grid->addWidget(b, 2, col);
        auto *e = new QLineEdit("Some typed text"); grid->addWidget(e, 3, col);
        auto *c = new QCheckBox("Check me"); c->setChecked(true); grid->addWidget(c, 4, col);
        if (!en) { l->setEnabled(false); b->setEnabled(false); e->setEnabled(false); c->setEnabled(false); }
    }
    w.resize(420, 220);
    w.show();
    app.processEvents();
    bool ok = w.grab().save(argc > 1 ? argv[1] : "widgets.png");
    std::printf("saved png: %d\n", ok);
    return 0;
}
