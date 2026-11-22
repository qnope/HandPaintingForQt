#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <cstdio>

class RectWidget : public QWidget {
public:
    RectWidget() { resize(100, 80); move(0, 0); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setPen(QPen(Qt::black, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(10, 10, 80, 60);
    }
};

static void report(const char* name, const QImage& img, const hpq::Theme& t) {
    auto bg = t.background().rgb();
    auto ink = t.ink();
    int non_bg_in = 0, non_bg_out = 0, near_edges = 0;
    QRect r(60, 60, 80, 60);
    int minx=1e9,miny=1e9,maxx=-1,maxy=-1;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x) {
            QRgb c = img.pixel(x, y);
            if (c == bg) continue;
            minx=std::min(minx,x);miny=std::min(miny,y);maxx=std::max(maxx,x);maxy=std::max(maxy,y);
            bool nearEdge = (std::abs(x - r.left()) <= 4 || std::abs(x - (r.left()+r.width())) <= 4) && y >= r.top()-4 && y <= r.top()+r.height()+4;
            nearEdge = nearEdge || ((std::abs(y - r.top()) <= 4 || std::abs(y - (r.top()+r.height())) <= 4) && x >= r.left()-4 && x <= r.left()+r.width()+4);
            if (nearEdge) ++near_edges;
            else if (x > r.left()+4 && x < r.right()-4 && y > r.top()+4 && y < r.bottom()-4) ++non_bg_in;
            else ++non_bg_out;
        }
    std::printf("%s: bg=%s ink=%s px(20,20)=%s px(100,90 center)=%s bbox non-bg=(%d,%d)-(%d,%d) near-edges=%d inside=%d elsewhere=%d\n",
        name, qPrintable(t.background().name()), qPrintable(ink.name()),
        qPrintable(QColor(img.pixel(20,20)).name()), qPrintable(QColor(img.pixel(100,90)).name()),
        minx,miny,maxx,maxy, near_edges, non_bg_in, non_bg_out);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    struct P { const char* name; hpq::Theme theme; };
    P presets[] = {{"sketch", hpq::Theme::sketch()}, {"china_ink", hpq::Theme::china_ink()}, {"chalk", hpq::Theme::chalk()}};
    for (auto& p : presets) {
        QImage img(200, 200, QImage::Format_ARGB32);
        img.fill(Qt::magenta);
        {
            hpq::HandPaintDevice device(img, p.theme);
            RectWidget w;
            w.render(&device, QPoint(50, 50), QRegion(), QWidget::DrawChildren);
        }
        report(p.name, img, p.theme);
        img.save(QString("b21_%1.png").arg(p.name));
        img.scaled(600, 600, Qt::KeepAspectRatio, Qt::FastTransformation).save(QString("b21_%1_x3.png").arg(p.name));
    }
    // Curiosity: real widget tree
    QFrame frame;
    frame.setFrameStyle(QFrame::Box | QFrame::Plain);
    frame.resize(260, 180);
    auto* lay = new QVBoxLayout(&frame);
    lay->addWidget(new QLabel("Hello label"));
    lay->addWidget(new QPushButton("Push me"));
    auto* cb = new QCheckBox("Check me"); cb->setChecked(true); lay->addWidget(cb);
    auto* le = new QLineEdit("some text"); lay->addWidget(le);
    frame.ensurePolished();
    lay->activate();
    for (auto& p : {presets[0], presets[2]}) {
        for (int f = 0; f < 2; ++f) {
            QImage img(300, 220, QImage::Format_ARGB32);
            img.fill(Qt::magenta);
            {
                hpq::HandPaintDevice device(img, p.theme);
                if (f == 0) frame.render(&device, QPoint(20, 20));
                else frame.render(&device, QPoint(20, 20), QRegion(), QWidget::DrawChildren);
            }
            img.save(QString("tree_%1_%2.png").arg(p.name).arg(f == 0 ? "default" : "children"));
        }
    }
    std::printf("done\n");
    return 0;
}
