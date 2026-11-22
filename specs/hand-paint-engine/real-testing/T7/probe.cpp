// Real-test probe for T7 (solid fill with the theme fill color), re-run after the PrimitiveTransform fix.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using Draw = std::function<void(QPainter&)>;

static QImage render(const hpq::Theme& theme, const Draw& draw, bool antialias = false)
{
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(Qt::red);
    {
        hpq::HandPaintDevice device(image, theme);
        QPainter painter(&device);
        if (antialias)
            painter.setRenderHint(QPainter::Antialiasing, true);
        draw(painter);
    }
    return image;
}

static bool near(QColor a, QColor b, int tol)
{
    return std::abs(a.red() - b.red()) <= tol && std::abs(a.green() - b.green()) <= tol
        && std::abs(a.blue() - b.blue()) <= tol;
}

static std::string hex(QColor c) { return c.name(QColor::HexArgb).toStdString(); }
static std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%g", v); return b; }

static double rect_outside_dist(const QRectF& r, int x, int y)
{
    double px = x + 0.5, py = y + 0.5;
    double dx = std::max({r.left() - px, 0.0, px - r.right()});
    double dy = std::max({r.top() - py, 0.0, py - r.bottom()});
    return std::hypot(dx, dy);
}

static int failures = 0;
static void check(bool ok, const std::string& what)
{
    std::printf("  [%s] %s\n", ok ? "ok" : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

struct Ctx { const hpq::Theme& theme; double D; const QImage& img; };
using Check = std::function<void(const Ctx&)>;

static QColor blend(const hpq::Theme& t, double a)
{
    QColor f = t.fill(), b = t.background();
    return QColor(int(std::lround(f.red() * a + b.red() * (1 - a))), int(std::lround(f.green() * a + b.green() * (1 - a))),
                  int(std::lround(f.blue() * a + b.blue() * (1 - a))));
}

// row y: first/last non-background x
static std::pair<int, int> row_extent(const Ctx& c, int y)
{
    int first = -1, last = -1;
    for (int x = 0; x < 200; ++x)
        if (c.img.pixelColor(x, y) != c.theme.background()) { if (first < 0) first = x; last = x; }
    return {first, last};
}
static std::pair<int, int> col_extent(const Ctx& c, int x)
{
    int first = -1, last = -1;
    for (int y = 0; y < 200; ++y)
        if (c.img.pixelColor(x, y) != c.theme.background()) { if (first < 0) first = y; last = y; }
    return {first, last};
}

static int count_if_px(const Ctx& c, const std::function<bool(int, int, QColor)>& f)
{
    int n = 0;
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 200; ++x)
            if (f(x, y, c.img.pixelColor(x, y))) ++n;
    return n;
}

static int brush_pen_colored(const Ctx& c)
{
    // user colors: blue, magenta, yellow, red-pen-ish (not theme colors of any preset)
    return count_if_px(c, [](int, int, QColor p) {
        const bool blue = p.blue() > p.red() + 60 && p.blue() > p.green() + 60;
        const bool magenta = p.red() > 200 && p.green() < 80 && p.blue() > 200;
        const bool yellow = p.red() > 200 && p.green() > 200 && p.blue() < 80;
        const bool red = p.red() > 200 && p.green() < 60 && p.blue() < 60;
        return blue || magenta || yellow || red;
    });
}

// outline thickness: on row y, run length of "ink-ish" (not bg, not fill) pixels starting near x0
static int ink_run(const Ctx& c, int y, int x_from, int x_to)
{
    int best = 0, cur = 0;
    for (int x = x_from; x <= x_to; ++x) {
        QColor p = c.img.pixelColor(x, y);
        if (p != c.theme.background() && p != c.theme.fill()) { ++cur; best = std::max(best, cur); } else cur = 0;
    }
    return best;
}

static void check_rect_like(const Ctx& c, const QRectF& ideal, const std::string& what)
{
    int bad = count_if_px(c, [&](int x, int y, QColor p) { return rect_outside_dist(ideal, x, y) > c.D && p != c.theme.background(); });
    check(bad == 0, what + ": pixels farther than D outside ideal rect (" + num(ideal.left()) + "," + num(ideal.top()) + ")-(" +
                        num(ideal.right()) + "," + num(ideal.bottom()) + ") not background: " + std::to_string(bad));
    auto [l, r] = row_extent(c, int(ideal.center().y()));
    check(l >= 0 && std::abs(l - ideal.left()) <= c.D + 1 && std::abs(r + 1 - ideal.right()) <= c.D + 1,
          what + ": row " + std::to_string(int(ideal.center().y())) + " non-bg x " + std::to_string(l) + ".." + std::to_string(r) +
              " (ideal " + num(ideal.left()) + ".." + num(ideal.right()) + ")");
    auto [t, b] = col_extent(c, int(ideal.center().x()));
    check(t >= 0 && std::abs(t - ideal.top()) <= c.D + 1 && std::abs(b + 1 - ideal.bottom()) <= c.D + 1,
          what + ": column " + std::to_string(int(ideal.center().x())) + " non-bg y " + std::to_string(t) + ".." + std::to_string(b) +
              " (ideal " + num(ideal.top()) + ".." + num(ideal.bottom()) + ")");
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const std::string out = argc > 1 ? argv[1] : ".";

    struct Named { const char* name; hpq::Theme theme; };
    std::vector<Named> themes{{"sketch", hpq::Theme::sketch()}, {"china_ink", hpq::Theme::china_ink()}, {"chalk", hpq::Theme::chalk()}};

    const QRectF rect(40, 40, 120, 120);
    const QPolygonF tri({QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)});
    const QPolygonF tri_half({QPointF(50, 15), QPointF(85, 85), QPointF(15, 85)});
    QPainterPath path; // D-shaped closed path, x 40..~150, y 40..160
    path.moveTo(40, 40); path.lineTo(110, 40); path.quadTo(170, 100, 110, 160); path.lineTo(40, 160); path.closeSubpath();
    QPainterPath path_half;
    path_half.moveTo(20, 20); path_half.lineTo(55, 20); path_half.quadTo(85, 50, 55, 80); path_half.lineTo(20, 80); path_half.closeSubpath();
    QImage red40(40, 40, QImage::Format_ARGB32); red40.fill(QColor(255, 0, 0));
    const QFont font("Helvetica", 20);

    struct Panel { std::string file; std::string label; Draw draw; bool aa; Check check; };
    std::vector<Panel> panels;

    // ---- B11/B12/B13 ----
    Draw b11 = [&](QPainter& p) { p.setPen(QPen(Qt::black)); p.setBrush(QBrush(Qt::blue)); p.drawRect(rect); };
    Check b11c = [&](const Ctx& c) {
        QColor px = c.img.pixelColor(100, 100);
        check(px == c.theme.fill(), "pixel(100,100)=" + hex(px) + " expected fill " + hex(c.theme.fill()));
        int bad = count_if_px(c, [&](int x, int y, QColor p) { return rect_outside_dist(rect, x, y) > c.D && p != c.theme.background(); });
        check(bad == 0, "pixels farther than D outside rect not background: " + std::to_string(bad));
        check(brush_pen_colored(c) == 0, "user brush/pen colored pixels: " + std::to_string(brush_pen_colored(c)));
        int missing = 0, samples = 0;
        for (int k = 0; k <= 480; k += 4) {
            double px2, py;
            if (k < 120) { px2 = 40 + k; py = 40; } else if (k < 240) { px2 = 160; py = 40 + (k - 120); }
            else if (k < 360) { px2 = 160 - (k - 240); py = 160; } else { px2 = 40; py = 160 - (k - 360); }
            ++samples;
            bool found = false;
            for (int dy = -int(c.D) - 1; dy <= int(c.D) + 1 && !found; ++dy)
                for (int dx = -int(c.D) - 1; dx <= int(c.D) + 1 && !found; ++dx) {
                    int x = int(px2) + dx, y = int(py) + dy;
                    if (x < 0 || y < 0 || x >= 200 || y >= 200) continue;
                    if (std::hypot(x + 0.5 - px2, y + 0.5 - py) > c.D + 0.5) continue;
                    QColor p = c.img.pixelColor(x, y);
                    if (p != c.theme.background() && p != c.theme.fill()) found = true;
                }
            if (!found) ++missing;
        }
        check(missing == 0, "outline samples with ink within D: " + std::to_string(samples - missing) + "/" + std::to_string(samples));
        QImage plain(200, 200, QImage::Format_ARGB32);
        plain.fill(c.theme.background());
        { QPainter pp(&plain); pp.setRenderHint(QPainter::Antialiasing);
          pp.setPen(QPen(c.theme.ink(), c.theme.stroke_width())); pp.setBrush(c.theme.fill()); pp.drawRect(rect); }
        check(c.img != plain, "differs from a plain QPainter rect");
    };
    Draw b12 = [&](QPainter& p) { p.setPen(Qt::NoPen); p.setBrush(QBrush(QColor(0, 0, 255, 128))); p.drawRect(rect); };
    Check b12c = [&](const Ctx& c) {
        QColor e = blend(c.theme, 128.0 / 255.0), px = c.img.pixelColor(100, 100);
        check(near(px, e, 3), "pixel(100,100)=" + hex(px) + " expected ~" + hex(e) + " (+-3)");
        int ink = count_if_px(c, [&](int, int, QColor p) { return p == c.theme.ink(); });
        check(ink == 0, "ink-color pixels: " + std::to_string(ink));
        int outside = count_if_px(c, [&](int x, int y, QColor p) { return rect_outside_dist(rect, x, y) > 1 && p != c.theme.background(); });
        check(outside == 0, "non-bg pixels >1px outside rect: " + std::to_string(outside));
    };
    Draw b13 = [&](QPainter& p) {
        p.setPen(Qt::NoPen); p.setBrush(Qt::NoBrush);
        p.drawRect(rect); p.drawEllipse(QRectF(30, 50, 140, 100)); p.drawPolygon(tri); p.drawPath(path);
    };
    Check b13c = [&](const Ctx& c) {
        int n = count_if_px(c, [&](int, int, QColor p) { return p != c.theme.background(); });
        check(n == 0, "non-background pixels: " + std::to_string(n));
    };
    panels.push_back({"B11", "B11 rect brush+pen", b11, false, b11c});
    panels.push_back({"B11_aa", "B11 + Antialiasing", b11, true, b11c});
    panels.push_back({"B12", "B12 alpha128 NoPen", b12, false, b12c});
    panels.push_back({"B12_aa", "B12 + Antialiasing", b12, true, b12c});
    panels.push_back({"B13", "B13 NoPen NoBrush", b13, false, b13c});

    // ---- translate(30,0), previously moved 60 px ----
    panels.push_back({"tr_rect_a128", "tr(30,0) rect a128 NoPen",
        [&](QPainter& p) { p.translate(30, 0); p.setPen(Qt::NoPen); p.setBrush(QBrush(QColor(0, 0, 255, 128))); p.drawRect(rect); }, false,
        [&](const Ctx& c) {
            QColor e = blend(c.theme, 128.0 / 255.0);
            for (QPoint q : {QPoint(75, 100), QPoint(130, 100), QPoint(185, 100)})
                check(near(c.img.pixelColor(q), e, 3), "pixel(" + std::to_string(q.x()) + ",100)=" + hex(c.img.pixelColor(q)) + " expected ~" + hex(e));
            check(c.img.pixelColor(65, 100) == c.theme.background() && c.img.pixelColor(195, 100) == c.theme.background(),
                  "pixel(65,100)=" + hex(c.img.pixelColor(65, 100)) + " pixel(195,100)=" + hex(c.img.pixelColor(195, 100)) + " expected bg");
            auto [l, r] = row_extent(c, 100);
            check(l == 70 && r == 189, "row 100 non-bg x " + std::to_string(l) + ".." + std::to_string(r) + " (ideal 70..189)");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
        }});
    panels.push_back({"tr_ellipse_a128", "tr(30,0) ellipse a128 + pen",
        [&](QPainter& p) { p.translate(30, 0); p.setPen(QPen(Qt::black)); p.setBrush(QBrush(QColor(0, 0, 255, 128))); p.drawEllipse(QRectF(30, 50, 140, 100)); }, true,
        [&](const Ctx& c) {
            QColor e = blend(c.theme, 128.0 / 255.0);
            check(near(c.img.pixelColor(130, 100), e, 3), "center (130,100)=" + hex(c.img.pixelColor(130, 100)) + " expected ~" + hex(e));
            check(near(c.img.pixelColor(70, 100), e, 3), "(70,100) inside translated ellipse=" + hex(c.img.pixelColor(70, 100)) + " expected ~" + hex(e));
            auto [l, r] = row_extent(c, 100);
            check(l >= 0 && std::abs(l - 60) <= c.D + 1, "row 100 left edge x=" + std::to_string(l) + " (ideal 60, within D)");
            auto [t, b] = col_extent(c, 130);
            check(t >= 0 && std::abs(t - 50) <= c.D + 1 && std::abs(b + 1 - 150) <= c.D + 1,
                  "column 130 non-bg y " + std::to_string(t) + ".." + std::to_string(b) + " (ideal 50..150)");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
        }});
    panels.push_back({"tr_rect_opaque", "tr(30,0) rect brush+pen",
        [&](QPainter& p) { p.translate(30, 0); p.setPen(QPen(Qt::black)); p.setBrush(QBrush(Qt::blue)); p.drawRect(rect); }, false,
        [&](const Ctx& c) {
            check(c.img.pixelColor(130, 100) == c.theme.fill(), "center (130,100)=" + hex(c.img.pixelColor(130, 100)) + " expected fill");
            check_rect_like(c, QRectF(70, 40, 120, 120), "translated rect");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
        }});
    panels.push_back({"tr_polygon", "tr(30,0) triangle yellow brush red pen",
        [&](QPainter& p) { p.translate(30, 0); p.setPen(QPen(Qt::red)); p.setBrush(QBrush(Qt::yellow)); p.drawPolygon(tri); }, true,
        [&](const Ctx& c) {
            check(c.img.pixelColor(130, 120) == c.theme.fill(), "(130,120)=" + hex(c.img.pixelColor(130, 120)) + " expected fill " + hex(c.theme.fill()));
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
            auto [l, r] = row_extent(c, 165);
            check(l >= 0 && std::abs(l - 62.5) <= c.D + 2 && std::abs(r + 1 - 197.5) <= c.D + 2,
                  "row 165 non-bg x " + std::to_string(l) + ".." + std::to_string(r) + " (ideal ~62.5..197.5)");
            auto [t, b] = col_extent(c, 130);
            check(t >= 0 && std::abs(t - 30) <= c.D + 2, "column 130 top y=" + std::to_string(t) + " (ideal apex y 30)");
            int left = count_if_px(c, [&](int x, int, QColor p) { return x < 60 - c.D - 1 && p != c.theme.background(); });
            check(left == 0, "non-bg pixels left of x=60-D: " + std::to_string(left));
        }});
    panels.push_back({"tr_path", "tr(30,0) path magenta brush",
        [&](QPainter& p) { p.translate(30, 0); p.setPen(QPen(Qt::darkGreen, 4)); p.setBrush(QBrush(Qt::magenta)); p.drawPath(path); }, true,
        [&](const Ctx& c) {
            check(c.img.pixelColor(120, 100) == c.theme.fill(), "(120,100)=" + hex(c.img.pixelColor(120, 100)) + " expected fill " + hex(c.theme.fill()));
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
            auto [l, r] = row_extent(c, 100);
            check(l >= 0 && std::abs(l - 70) <= c.D + 1 && std::abs(r + 1 - 170) <= c.D + 2,
                  "row 100 non-bg x " + std::to_string(l) + ".." + std::to_string(r) + " (ideal 70..170)");
        }});

    // ---- scale(2,2) ----
    panels.push_back({"sc_rect", "scale(2,2) rect(20,20,60,60) brush+pen",
        [&](QPainter& p) { p.scale(2, 2); p.setPen(QPen(Qt::black)); p.setBrush(QBrush(Qt::blue)); p.drawRect(QRectF(20, 20, 60, 60)); }, false,
        [&](const Ctx& c) {
            check(c.img.pixelColor(100, 100) == c.theme.fill(), "center (100,100)=" + hex(c.img.pixelColor(100, 100)) + " expected fill");
            check_rect_like(c, QRectF(40, 40, 120, 120), "scaled rect");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
            // stroke thickness on the left edge, a few rows
            int maxrun = 0;
            for (int y : {60, 80, 100, 120, 140}) maxrun = std::max(maxrun, ink_run(c, y, 30, 55));
            std::printf("  [info] left edge ink run length (max over 5 rows) = %d px, theme stroke width %s\n", maxrun, num(c.theme.stroke_width()).c_str());
            check(maxrun <= int(std::ceil(c.theme.stroke_width())) + 3, "stroke not doubled: run " + std::to_string(maxrun) + " <= stroke_width+3");
        }});
    panels.push_back({"tr_rect_ref", "reference: rect(40,40,120,120) brush+pen, no transform",
        b11, false, [&](const Ctx& c) {
            int maxrun = 0;
            for (int y : {60, 80, 100, 120, 140}) maxrun = std::max(maxrun, ink_run(c, y, 30, 55));
            std::printf("  [info] untransformed left edge ink run length = %d px\n", maxrun);
        }});
    panels.push_back({"sc_polygon", "scale(2,2) triangle yellow brush",
        [&](QPainter& p) { p.scale(2, 2); p.setPen(QPen(Qt::red)); p.setBrush(QBrush(Qt::yellow)); p.drawPolygon(tri_half); }, true,
        [&](const Ctx& c) {
            check(c.img.pixelColor(100, 120) == c.theme.fill(), "(100,120)=" + hex(c.img.pixelColor(100, 120)) + " expected fill");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
            auto [l, r] = row_extent(c, 165);
            check(l >= 0 && std::abs(l - 32.5) <= c.D + 2 && std::abs(r + 1 - 167.5) <= c.D + 2,
                  "row 165 non-bg x " + std::to_string(l) + ".." + std::to_string(r) + " (ideal ~32.5..167.5)");
            auto [t, b] = col_extent(c, 100);
            check(t >= 0 && std::abs(t - 30) <= c.D + 2 && std::abs(b + 1 - 170) <= c.D + 2,
                  "column 100 non-bg y " + std::to_string(t) + ".." + std::to_string(b) + " (ideal 30..170)");
        }});
    panels.push_back({"sc_path", "scale(2,2) path magenta brush",
        [&](QPainter& p) { p.scale(2, 2); p.setPen(QPen(Qt::darkGreen, 4)); p.setBrush(QBrush(Qt::magenta)); p.drawPath(path_half); }, true,
        [&](const Ctx& c) {
            check(c.img.pixelColor(100, 100) == c.theme.fill(), "(100,100)=" + hex(c.img.pixelColor(100, 100)) + " expected fill");
            check(brush_pen_colored(c) == 0, "user-colored pixels: " + std::to_string(brush_pen_colored(c)));
            auto [l, r] = row_extent(c, 100);
            check(l >= 0 && std::abs(l - 40) <= c.D + 1 && std::abs(r + 1 - 140) <= c.D + 2,
                  "row 100 non-bg x " + std::to_string(l) + ".." + std::to_string(r) + " (ideal 40..140)");
            auto [t, b] = col_extent(c, 70);
            check(t >= 0 && std::abs(t - 40) <= c.D + 1 && std::abs(b + 1 - 160) <= c.D + 1,
                  "column 70 non-bg y " + std::to_string(t) + ".." + std::to_string(b) + " (ideal 40..160)");
        }});

    // ---- pass-through after translate ----
    panels.push_back({"tr_text", "tr(30,0) drawText(20,100) red pen",
        [&](QPainter& p) { p.translate(30, 0); p.setFont(font); p.setPen(QPen(Qt::red)); p.drawText(QPointF(20, 100), "Hello"); }, false,
        [&](const Ctx& c) {
            QImage plain(200, 200, QImage::Format_ARGB32);
            plain.fill(c.theme.background());
            { QPainter pp(&plain); pp.translate(30, 0); pp.setFont(font); pp.setPen(c.theme.ink()); pp.drawText(QPointF(20, 100), "Hello"); }
            int diff = 0;
            for (int y = 0; y < 200; ++y) for (int x = 0; x < 200; ++x) if (plain.pixel(x, y) != c.img.pixel(x, y)) ++diff;
            int minx = 200;
            for (int y = 0; y < 200; ++y) for (int x = 0; x < 200; ++x) if (c.img.pixelColor(x, y) != c.theme.background()) minx = std::min(minx, x);
            check(diff == 0, "identical to plain QPainter text at translate(30,0): differing pixels " + std::to_string(diff) +
                                 ", leftmost text pixel x=" + std::to_string(minx) + " (expected ~50, not ~80)");
        }});
    auto image_check = [&](const Ctx& c) {
        int diff = 0;
        for (int y = 80; y < 120; ++y) for (int x = 110; x < 150; ++x) if (c.img.pixelColor(x, y) != QColor(255, 0, 0)) ++diff;
        check(diff == 0, "region (110,80)-(150,120) equals the source: differing pixels " + std::to_string(diff));
        int outside = count_if_px(c, [&](int x, int y, QColor p) {
            const bool in = x >= 110 && x < 150 && y >= 80 && y < 120;
            return !in && p != c.theme.background();
        });
        check(outside == 0, "non-bg pixels outside (110,80)-(150,120): " + std::to_string(outside) + " (twice-translated copy would be at 140..180)");
    };
    panels.push_back({"tr_image", "tr(30,0) drawImage(80,80) 40x40 red",
        [&](QPainter& p) { p.translate(30, 0); p.drawImage(QPointF(80, 80), red40); }, false, image_check});
    panels.push_back({"tr_pixmap", "tr(30,0) drawPixmap(80,80) 40x40 red",
        [&](QPainter& p) { p.translate(30, 0); p.drawPixmap(QPointF(80, 80), QPixmap::fromImage(red40)); }, false, image_check});

    // ---- render, check, sheets ----
    const int cols = 6, cell = 260, label_h = 34;
    const int rows = (int(panels.size()) + cols - 1) / cols;
    for (size_t t = 0; t < themes.size(); ++t) {
        const auto& theme = themes[t].theme;
        const double D = theme.roughness() + theme.stroke_width();
        std::printf("== %s (bg %s ink %s fill %s stroke %s D=%s)\n", themes[t].name, hex(theme.background()).c_str(),
                    hex(theme.ink()).c_str(), hex(theme.fill()).c_str(), num(theme.stroke_width()).c_str(), num(D).c_str());
        QImage sheet(cols * cell, rows * (cell + label_h), QImage::Format_ARGB32);
        sheet.fill(Qt::white);
        QPainter sp(&sheet);
        sp.setFont(QFont("Helvetica", 10));
        for (size_t i = 0; i < panels.size(); ++i) {
            const auto& pn = panels[i];
            QImage img = render(theme, pn.draw, pn.aa);
            std::printf(" %s\n", pn.label.c_str());
            pn.check(Ctx{theme, D, img});
            img.save(QString::fromStdString(out + "/" + themes[t].name + "_" + pn.file + ".png"));
            int ox = int(i % cols) * cell, oy = int(i / cols) * (cell + label_h);
            sp.setPen(Qt::black);
            sp.drawText(QRect(ox + 2, oy, cell - 4, label_h), Qt::AlignCenter | Qt::TextWordWrap, QString::fromStdString(pn.label));
            QRect target(ox + 30, oy + label_h + 30, 200, 200);
            sp.drawImage(target, img);
            sp.setPen(Qt::gray);
            sp.drawRect(target.adjusted(-1, -1, 0, 0));
            // ruler ticks every 10 px (long every 50)
            for (int k = 0; k <= 200; k += 10) {
                int len = k % 50 == 0 ? 8 : 4;
                sp.drawLine(target.left() + k, target.top() - 1 - len, target.left() + k, target.top() - 1);
                sp.drawLine(target.left() - 1 - len, target.top() + k, target.left() - 1, target.top() + k);
                if (k % 50 == 0) {
                    sp.drawText(QPointF(target.left() + k - 6, target.top() - 12), QString::number(k));
                    sp.drawText(QPointF(target.left() - 28, target.top() + k + 4), QString::number(k));
                }
            }
        }
        sp.end();
        sheet.save(QString::fromStdString(out + "/sheet_" + themes[t].name + ".png"));
    }
    std::printf("failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
