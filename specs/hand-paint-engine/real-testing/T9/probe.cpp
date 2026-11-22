// Real-test probe for T9 (B16): painter transforms on hpq::HandPaintDevice.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>

#include <cstdio>
#include <optional>
#include <string>

namespace {

QImage blank() { return QImage(200, 200, QImage::Format_ARGB32_Premultiplied); }

bool is_bg(const QImage& img, int x, int y, QColor bg) { return img.pixelColor(x, y).rgb() == bg.rgb(); }

struct Box { int x0 = 9999, y0 = 9999, x1 = -1, y1 = -1; int n = 0; };

Box ink_box(const QImage& img, QColor bg)
{
    Box b;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (!is_bg(img, x, y, bg)) {
                ++b.n;
                b.x0 = std::min(b.x0, x); b.y0 = std::min(b.y0, y);
                b.x1 = std::max(b.x1, x); b.y1 = std::max(b.y1, y);
            }
    return b;
}

// Number of non-bg pixels in a rectangular zone.
int ink_in(const QImage& img, QColor bg, int x0, int y0, int x1, int y1)
{
    int n = 0;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (!is_bg(img, x, y, bg)) ++n;
    return n;
}

// Mean horizontal run length of dark (strong ink) pixels per row across a band of rows, x in [x0,x1].
double mean_run(const QImage& img, QColor ink, int y0, int y1, int x0, int x1)
{
    double total = 0; int rows = 0;
    for (int y = y0; y <= y1; ++y) {
        int n = 0;
        for (int x = x0; x <= x1; ++x) {
            const QColor c = img.pixelColor(x, y);
            const int d = std::abs(c.red() - ink.red()) + std::abs(c.green() - ink.green()) + std::abs(c.blue() - ink.blue());
            if (d < 200) ++n;
        }
        if (n > 0) { total += n; ++rows; }
    }
    return rows ? total / rows : 0;
}

void print_box(const char* name, const Box& b)
{
    std::printf("%s: ink pixels=%d bbox=(%d,%d)-(%d,%d)\n", name, b.n, b.x0, b.y0, b.x1, b.y1);
}

QPen theme_pen() { QPen p(Qt::black); p.setWidth(1); return p; }

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";

    const hpq::Theme sketch = hpq::Theme::sketch();
    const hpq::Theme ink = hpq::Theme::china_ink();

    // 1. translate(50,0) then drawRect(20,40,80,80)
    {
        QImage img = blank();
        hpq::HandPaintDevice dev(img, sketch);
        QPainter p(&dev);
        p.setPen(theme_pen()); p.setBrush(Qt::NoBrush);
        p.translate(50, 0);
        p.drawRect(20, 40, 80, 80);
        p.end();
        img.save(QString::fromStdString(out + "/b16_translate.png"));
        print_box("translate", ink_box(img, sketch.background()));
        std::printf("translate: pixel(30,80) bg=%d; ink in untransformed left edge zone x[15,25] y[50,110]=%d; "
                    "ink at new left edge x[65,75] y[50,110]=%d; right edge x[145,155]=%d; interior x[85,135] y[60,100]=%d\n",
                    is_bg(img, 30, 80, sketch.background()), ink_in(img, sketch.background(), 15, 50, 25, 110),
                    ink_in(img, sketch.background(), 65, 50, 75, 110), ink_in(img, sketch.background(), 145, 50, 155, 110),
                    ink_in(img, sketch.background(), 85, 60, 135, 100));
    }

    // 2. scale(2,2) then drawRect(20,20,40,40) vs. untransformed drawRect(40,40,80,80); China Ink (stroke 2.5)
    double run_scaled = 0, run_plain = 0;
    for (int scaled = 0; scaled < 2; ++scaled) {
        QImage img = blank();
        hpq::HandPaintDevice dev(img, ink);
        QPainter p(&dev);
        p.setPen(theme_pen()); p.setBrush(Qt::NoBrush);
        if (scaled) { p.scale(2, 2); p.drawRect(20, 20, 40, 40); }
        else p.drawRect(40, 40, 80, 80);
        p.end();
        const char* name = scaled ? "scale" : "scale_reference_unscaled";
        img.save(QString::fromStdString(out + "/b16_" + name + ".png"));
        print_box(name, ink_box(img, ink.background()));
        const double run = mean_run(img, ink.ink(), 60, 100, 25, 55); // left edge stroke thickness
        (scaled ? run_scaled : run_plain) = run;
        std::printf("%s: mean dark run across left edge=%.2f px; ink at untransformed rect zone x[15,25] y[25,55]=%d; interior x[55,105] y[55,105]=%d\n",
                    name, run, ink_in(img, ink.background(), 15, 25, 25, 55), ink_in(img, ink.background(), 55, 55, 105, 105));
    }
    std::printf("scale: stroke ratio scaled/unscaled=%.2f\n", run_scaled / run_plain);

    // 3. rotate(30) about the center, with plain QPainter reference
    {
        QImage img = blank();
        hpq::HandPaintDevice dev(img, sketch);
        QPainter p(&dev);
        p.setPen(theme_pen()); p.setBrush(Qt::NoBrush);
        p.translate(100, 100); p.rotate(30); p.translate(-100, -100);
        p.drawRect(50, 50, 100, 100);
        p.end();
        img.save(QString::fromStdString(out + "/b16_rotate.png"));
        print_box("rotate", ink_box(img, sketch.background()));
        QImage ref = blank(); ref.fill(Qt::white);
        QPainter r(&ref);
        r.setRenderHint(QPainter::Antialiasing);
        r.translate(100, 100); r.rotate(30); r.translate(-100, -100);
        r.drawRect(50, 50, 100, 100);
        r.end();
        ref.save(QString::fromStdString(out + "/b16_rotate_reference_qpainter.png"));
        print_box("rotate_reference_qpainter", ink_box(ref, Qt::white));
        std::printf("rotate: ink near untransformed top-left corner (50,50) zone x[45,55] y[45,55]=%d\n",
                    ink_in(img, sketch.background(), 45, 45, 55, 55));
    }

    // 4. save/restore between shapes
    {
        QImage img = blank();
        hpq::HandPaintDevice dev(img, sketch);
        QPainter p(&dev);
        p.setPen(theme_pen()); p.setBrush(Qt::NoBrush);
        p.save();
        p.translate(100, 0);
        p.drawRect(10, 10, 60, 60);   // at (110,10)
        p.restore();
        p.drawRect(10, 110, 60, 60);  // untransformed at (10,110)
        p.end();
        img.save(QString::fromStdString(out + "/b16_save_restore.png"));
        const QColor bg = sketch.background();
        std::printf("save_restore: ink TR quadrant x[100,199] y[0,99]=%d; BL quadrant x[0,99] y[100,199]=%d; "
                    "TL quadrant=%d; BR quadrant=%d\n",
                    ink_in(img, bg, 100, 0, 199, 99), ink_in(img, bg, 0, 100, 99, 199),
                    ink_in(img, bg, 0, 0, 99, 99), ink_in(img, bg, 100, 100, 199, 199));
    }

    // 5. drawText / drawImage after translate
    {
        QImage img = blank();
        hpq::HandPaintDevice dev(img, sketch);
        QPainter p(&dev);
        QImage red(30, 30, QImage::Format_ARGB32_Premultiplied); red.fill(QColor(255, 0, 0));
        p.translate(100, 100);
        p.drawImage(QPoint(10, 10), red);      // expected at (110..139, 110..139)
        QFont f("Helvetica"); f.setPixelSize(24); p.setFont(f);
        p.setPen(Qt::black);
        p.drawText(QPoint(-80, -40), "Hello"); // baseline at (20,60)
        p.end();
        img.save(QString::fromStdString(out + "/b16_text_image.png"));
        int red_n = 0; Box rb;
        for (int y = 0; y < 200; ++y)
            for (int x = 0; x < 200; ++x)
                if (img.pixel(x, y) == qRgb(255, 0, 0)) {
                    ++red_n; rb.x0 = std::min(rb.x0, x); rb.y0 = std::min(rb.y0, y);
                    rb.x1 = std::max(rb.x1, x); rb.y1 = std::max(rb.y1, y);
                }
        std::printf("text_image: red pixels=%d bbox=(%d,%d)-(%d,%d)\n", red_n, rb.x0, rb.y0, rb.x1, rb.y1);
        std::printf("text_image: ink in text zone x[15,100] y[35,65]=%d; ink at untransformed text zone x[0,40] y[0,30] (minus nothing)=%d\n",
                    ink_in(img, sketch.background(), 15, 35, 100, 65), ink_in(img, sketch.background(), 0, 0, 14, 30));
        // reference with plain QPainter
        QImage ref = blank(); ref.fill(sketch.background());
        QPainter r(&ref);
        r.translate(100, 100);
        r.drawImage(QPoint(10, 10), red);
        r.setFont(f); r.setPen(Qt::black);
        r.drawText(QPoint(-80, -40), "Hello");
        r.end();
        ref.save(QString::fromStdString(out + "/b16_text_image_reference_qpainter.png"));
        Box tb = ink_box(img, sketch.background()); Box tr = ink_box(ref, sketch.background());
        print_box("text_image (hpq)", tb); print_box("text_image (plain QPainter)", tr);
    }
        // 6. drawEllipse and drawPath(cubic) under translate(50,0) and scale(2,2), vs. untransformed equivalents
    {
        const QColor bg = sketch.background();
        auto shapes = [](QPainter& p, int mode) {
            // mode 0: translate(50,0) with ellipse (10,20,60,40) and cubic (0,140)->(90,140) ctrl (20,100),(70,180)
            // mode 1: untransformed equivalent of mode 0 (ellipse at 60,20 ; cubic shifted +50)
            // mode 2: scale(2,2) with ellipse (10,10,40,20) and cubic (5,70)->(50,70) ctrl (15,50),(35,90)
            // mode 3: untransformed equivalent of mode 2 (ellipse 20,20,80,40 ; cubic doubled)
            QPainterPath cubic;
            if (mode == 0) { p.translate(50, 0); p.drawEllipse(QRectF(10, 20, 60, 40));
                cubic.moveTo(0, 140); cubic.cubicTo(20, 100, 70, 180, 90, 140); }
            if (mode == 1) { p.drawEllipse(QRectF(60, 20, 60, 40));
                cubic.moveTo(50, 140); cubic.cubicTo(70, 100, 120, 180, 140, 140); }
            if (mode == 2) { p.scale(2, 2); p.drawEllipse(QRectF(10, 10, 40, 20));
                cubic.moveTo(5, 70); cubic.cubicTo(15, 50, 35, 90, 50, 70); }
            if (mode == 3) { p.drawEllipse(QRectF(20, 20, 80, 40));
                cubic.moveTo(10, 140); cubic.cubicTo(30, 100, 70, 180, 100, 140); }
            p.drawPath(cubic);
        };
        const char* names[] = {"curves_translate", "curves_translate_untransformed", "curves_scale", "curves_scale_untransformed"};
        for (int mode = 0; mode < 4; ++mode) {
            for (int plain = 0; plain < 2; ++plain) {
                QImage img = blank();
                if (plain) img.fill(bg);
                std::optional<hpq::HandPaintDevice> dev;
                QPaintDevice* target = &img;
                if (!plain) { dev.emplace(img, sketch); target = &*dev; }
                QPainter p(target);
                if (plain) p.setRenderHint(QPainter::Antialiasing);
                p.setPen(theme_pen()); p.setBrush(Qt::NoBrush);
                shapes(p, mode);
                p.end();
                const std::string nm = std::string(names[mode]) + (plain ? "_reference_qpainter" : "");
                img.save(QString::fromStdString(out + "/b16_" + nm + ".png"));
                print_box(nm.c_str(), ink_box(img, bg));
                if (!plain) {
                    std::printf("%s: ink top half y[0,99]=%d bottom half y[100,199]=%d; left strip x[0,%d]=%d\n", nm.c_str(),
                                ink_in(img, bg, 0, 0, 199, 99), ink_in(img, bg, 0, 100, 199, 199),
                                mode < 2 ? 45 : 5, ink_in(img, bg, 0, 0, mode < 2 ? 45 : 5, 199));
                }
            }
        }
    }
    return 0;
}
