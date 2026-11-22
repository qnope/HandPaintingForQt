// Real-user probe for hand-paint-engine T3 (B20): a user-provided GeometryEngine drives the drawing.
#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>

#include <cstdio>
#include <functional>
#include <memory>
#include <string>

namespace {

class IdentityEngine : public hpq::GeometryEngine
{
public:
    hpq::HandDrawnShape hand_draw(const QPainterPath& ideal, const hpq::Theme&, hpq::RandomGenerator&) const override
    {
        ++calls;
        return {ideal, ideal};
    }
    static inline int calls = 0;
};

// Shifts the strokes by (+25,0) and leaves the fill in place: shows that the engine output drives the drawing.
class ShiftStrokesEngine : public hpq::GeometryEngine
{
public:
    hpq::HandDrawnShape hand_draw(const QPainterPath& ideal, const hpq::Theme&, hpq::RandomGenerator&) const override
    {
        return {QTransform::fromTranslate(25, 0).map(ideal), ideal};
    }
};

QPolygonF triangle() { return QPolygonF({QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)}); }

QImage blank(const QColor& c)
{
    QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
    img.fill(c);
    return img;
}

QImage plain(const hpq::Theme& t, const std::function<void(QPainter&)>& draw, double dx = 0)
{
    QImage img = blank(t.background());
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(t.ink(), t.stroke_width()));
    p.setBrush(t.fill());
    p.translate(dx, 0);
    draw(p);
    return img;
}

QImage through_device(const hpq::Theme& t, std::unique_ptr<hpq::GeometryEngine> e,
                      const std::function<void(QPainter&)>& draw, bool with_brush = true)
{
    QImage img = blank(Qt::magenta); // the device must paint the background itself
    hpq::HandPaintDevice dev(img, t, std::move(e));
    QPainter p(&dev);
    p.setPen(QPen(Qt::red, 7));
    if (with_brush) p.setBrush(QBrush(Qt::blue));
    draw(p);
    p.end();
    return img;
}

int count_diff(const QImage& a, const QImage& b, int& maxd)
{
    int n = 0;
    maxd = 0;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x) {
            QRgb pa = a.pixel(x, y), pb = b.pixel(x, y);
            int d = std::max({std::abs(qRed(pa) - qRed(pb)), std::abs(qGreen(pa) - qGreen(pb)),
                              std::abs(qBlue(pa) - qBlue(pb)), std::abs(qAlpha(pa) - qAlpha(pb))});
            if (d) { ++n; maxd = std::max(maxd, d); }
        }
    return n;
}

int count_color(const QImage& img, QColor c)
{
    int n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (img.pixel(x, y) == c.rgba()) ++n;
    return n;
}

} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";
    struct Named { const char* name; hpq::Theme theme; };
    Named themes[] = {{"sketch", hpq::Theme::sketch()}, {"china_ink", hpq::Theme::china_ink()}, {"chalk", hpq::Theme::chalk()}};

    auto poly = [](QPainter& p) { p.drawPolygon(triangle()); };
    auto path = [](QPainter& p) { QPainterPath pp; pp.addPolygon(triangle()); pp.closeSubpath(); p.drawPath(pp); };

    int failures = 0;
    QImage sheet(200 * 4, 200 * 3, QImage::Format_ARGB32_Premultiplied);
    sheet.fill(Qt::gray);
    QPainter sp(&sheet);
    int row = 0;
    for (auto& n : themes) {
        IdentityEngine::calls = 0;
        QImage dev_poly = through_device(n.theme, std::make_unique<IdentityEngine>(), poly);
        const int poly_calls = IdentityEngine::calls;
        QImage ref = plain(n.theme, poly);
        QImage dev_path = through_device(n.theme, std::make_unique<IdentityEngine>(), path);
        QImage shifted = through_device(n.theme, std::make_unique<ShiftStrokesEngine>(), poly);
        int m1, m2, m3;
        int d1 = count_diff(dev_poly, ref, m1);
        int d2 = count_diff(dev_path, ref, m2);
        int d3 = count_diff(shifted, ref, m3);
        int red = count_color(dev_poly, Qt::red), blue = count_color(dev_poly, Qt::blue),
            magenta = count_color(dev_poly, Qt::magenta);
        std::printf("[%s] engine calls=%d | polygon vs plain: %d diff px (max %d) | path vs plain: %d diff px (max %d)"
                    " | shifted-engine vs plain: %d diff px | red=%d blue=%d magenta=%d\n",
                    n.name, poly_calls, d1, m1, d2, m2, d3, red, blue, magenta);
        if (d1 || d2 || !d3 || red || blue || magenta || poly_calls != 1) ++failures;
        dev_poly.save(QString::fromStdString(out + "/" + n.name + "_device_identity.png"));
        ref.save(QString::fromStdString(out + "/" + n.name + "_plain_qpainter.png"));
        dev_path.save(QString::fromStdString(out + "/" + n.name + "_device_identity_drawPath.png"));
        shifted.save(QString::fromStdString(out + "/" + n.name + "_device_shift_strokes.png"));
        sp.drawImage(0, row * 200, dev_poly);
        sp.drawImage(200, row * 200, ref);
        sp.drawImage(400, row * 200, dev_path);
        sp.drawImage(600, row * 200, shifted);
        ++row;
    }
    sp.setPen(QPen(Qt::darkGray, 2));
    for (int i = 1; i < 4; ++i) sp.drawLine(i * 200, 0, i * 200, 600);
    for (int i = 1; i < 3; ++i) sp.drawLine(0, i * 200, 800, i * 200);
    sp.end();
    sheet.save(QString::fromStdString(out + "/contact_sheet.png"));

    // No brush: the fill must not be painted.
    hpq::Theme s = hpq::Theme::sketch();
    QImage nobrush = through_device(s, std::make_unique<IdentityEngine>(), poly, false);
    QImage nobrush_ref = blank(s.background());
    {
        QPainter p(&nobrush_ref);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(s.ink(), s.stroke_width()));
        p.drawPolygon(triangle());
    }
    int m4;
    int d4 = count_diff(nobrush, nobrush_ref, m4);
    std::printf("[sketch no brush] vs plain outline-only: %d diff px (max %d)\n", d4, m4);
    nobrush.save(QString::fromStdString(out + "/sketch_no_brush_device.png"));

    if (d4) ++failures;

    // Text then polygon on the same device and the same painter: the polygon must still use theme ink/fill/width.
    QImage sheet2(200 * 2, 200 * 3, QImage::Format_ARGB32_Premultiplied);
    sheet2.fill(Qt::gray);
    QPainter sp2(&sheet2);
    row = 0;
    for (auto& n : themes) {
        auto text_then_poly = [](QPainter& p) {
            p.drawText(QPointF(20, 20), "Hello");
            p.drawPolygon(triangle());
        };
        QImage dev = through_device(n.theme, std::make_unique<IdentityEngine>(), text_then_poly);
        QImage ref = blank(n.theme.background());
        {
            QPainter p(&ref);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(n.theme.ink(), n.theme.stroke_width()));
            p.setBrush(n.theme.fill());
            text_then_poly(p);
        }
        // Reference for the polygon alone (ignoring the text box rows 0..30) to localise any difference.
        QImage ref_poly = plain(n.theme, poly);
        int m5, m6 = 0, d6 = 0;
        int d5 = count_diff(dev, ref, m5);
        for (int y = 30; y < 200; ++y)
            for (int x = 0; x < 200; ++x) {
                QRgb a = dev.pixel(x, y), b = ref_poly.pixel(x, y);
                int d = std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)), std::abs(qBlue(a) - qBlue(b))});
                if (d) { ++d6; m6 = std::max(m6, d); }
            }
        int red = count_color(dev, Qt::red), blue = count_color(dev, Qt::blue), magenta = count_color(dev, Qt::magenta);
        std::printf("[%s text+polygon] whole image vs plain text+polygon: %d diff px (max %d) | polygon area (y>=30) vs plain polygon: %d diff px (max %d)"
                    " | red=%d blue=%d magenta=%d | fill pixel (100,130)=%s theme fill=%s\n",
                    n.name, d5, m5, d6, m6, red, blue, magenta,
                    qPrintable(QColor(dev.pixel(100, 130)).name()), qPrintable(n.theme.fill().name()));
        if (d5 || d6 || red || blue || magenta) ++failures;
        dev.save(QString::fromStdString(out + "/" + n.name + "_text_then_polygon_device.png"));
        ref.save(QString::fromStdString(out + "/" + n.name + "_text_then_polygon_plain.png"));
        sp2.drawImage(0, row * 200, dev);
        sp2.drawImage(200, row * 200, ref);
        ++row;
    }
    sp2.setPen(QPen(Qt::darkGray, 2));
    sp2.drawLine(200, 0, 200, 600);
    for (int i = 1; i < 3; ++i) sp2.drawLine(0, i * 200, 400, i * 200);
    sp2.end();
    sheet2.save(QString::fromStdString(out + "/contact_sheet_text_then_polygon.png"));

    std::printf("%s\n", failures ? "PROBE: FAILURES" : "PROBE: all identity drawings equal the plain QPainter drawing");
    return failures ? 1 : 0;
}
