// Real-test probe for hand-paint-engine T6 (B7 ellipse, B10 path with a cubic).
// Draws through hpq::HandPaintDevice like a user, saves PNGs and prints measurements.
#include <hpq/HandPaintDevice.h>
#include <hpq/Theme.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

using Draw = std::function<void(QPainter&)>;

static QImage hand(const hpq::Theme& theme, const Draw& draw)
{
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(Qt::red);
    hpq::HandPaintDevice device(image, theme);
    QPainter painter(&device);
    painter.setPen(QPen(Qt::red, 1));
    painter.setBrush(Qt::NoBrush);
    draw(painter);
    painter.end();
    return image;
}

static QImage plain(const hpq::Theme& theme, const Draw& draw)
{
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(theme.background());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(theme.ink(), theme.stroke_width()));
    painter.setBrush(Qt::NoBrush);
    draw(painter);
    return image;
}

static std::vector<QPointF> sample(const QPainterPath& path)
{
    std::vector<QPointF> pts;
    for (const QPolygonF& poly : path.toSubpathPolygons())
    {
        for (int i = 0; i + 1 < poly.size(); ++i)
        {
            const QLineF l(poly[i], poly[i + 1]);
            const int n = std::max(1, int(std::ceil(l.length() / 0.1)));
            for (int k = 0; k <= n; ++k)
                pts.push_back(l.pointAt(double(k) / n));
        }
    }
    return pts;
}

static double dist(const std::vector<QPointF>& pts, QPointF p)
{
    double best = 1e9;
    for (const QPointF& q : pts)
        best = std::min(best, std::hypot(q.x() - p.x(), q.y() - p.y()));
    return best;
}

static bool between(QColor c, QColor bg, QColor ink)
{
    auto ok = [](int v, int a, int b) { return v >= std::min(a, b) - 2 && v <= std::max(a, b) + 2; };
    return ok(c.red(), bg.red(), ink.red()) && ok(c.green(), bg.green(), ink.green()) &&
           ok(c.blue(), bg.blue(), ink.blue());
}

static void measure(const char* name, const hpq::Theme& theme, const QPainterPath& ideal, const QImage& img,
                    const QImage& ref, QTextStream& out)
{
    const double d = theme.roughness() + theme.stroke_width();
    const std::vector<QPointF> pts = sample(ideal);
    double max_far = 0;
    int non_bg = 0, not_blend = 0, beyond_d = 0, red = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
        {
            const QColor c = img.pixelColor(x, y);
            if (c.rgb() == theme.background().rgb())
                continue;
            ++non_bg;
            if (!between(c, theme.background(), theme.ink()))
                ++not_blend;
            if (c.red() > 200 && c.green() < 60 && c.blue() < 60)
                ++red;
            const double dd = dist(pts, QPointF(x + 0.5, y + 0.5));
            max_far = std::max(max_far, dd);
            if (dd > d)
                ++beyond_d;
        }
    // coverage: every ideal outline point (sampled every 1px) has an ink-blend non-bg pixel within D
    int uncovered = 0, checked = 0;
    double worst_gap = 0;
    for (std::size_t i = 0; i < pts.size(); i += 10)
    {
        ++checked;
        const QPointF p = pts[i];
        double best = 1e9;
        const int r = int(std::ceil(d)) + 1;
        for (int y = int(p.y()) - r; y <= int(p.y()) + r; ++y)
            for (int x = int(p.x()) - r; x <= int(p.x()) + r; ++x)
            {
                if (x < 0 || y < 0 || x >= 200 || y >= 200)
                    continue;
                const QColor c = img.pixelColor(x, y);
                if (c.rgb() == theme.background().rgb() || !between(c, theme.background(), theme.ink()))
                    continue;
                best = std::min(best, std::hypot(x + 0.5 - p.x(), y + 0.5 - p.y()));
            }
        worst_gap = std::max(worst_gap, best);
        if (best > d)
            ++uncovered;
    }
    out << name << ": D=" << d << " nonBg=" << non_bg << " notInkBlend=" << not_blend << " red=" << red
        << " maxDistFromIdeal=" << max_far << " pixelsBeyondD=" << beyond_d << " outlineSamplesUncovered="
        << uncovered << "/" << checked << " worstNearestInk=" << worst_gap
        << " differsFromPlain=" << (img != ref ? "yes" : "NO") << "\n";
}

static QImage enlarge(const QImage& img, QRect crop, int factor)
{
    return img.copy(crop).scaled(crop.size() * factor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    const QString outdir = argc > 1 ? argv[1] : ".";
    QTextStream out(stdout);

    struct Named { const char* name; hpq::Theme theme; };
    const std::vector<Named> themes{{"sketch", hpq::Theme::sketch()},
                                    {"china_ink", hpq::Theme::china_ink()},
                                    {"chalk", hpq::Theme::chalk()}};

    QPainterPath curve_path;
    curve_path.moveTo(20, 150);
    curve_path.lineTo(80, 150);
    curve_path.cubicTo(QPointF(100, 20), QPointF(140, 20), QPointF(180, 150));
    QPainterPath circle;
    circle.addEllipse(QRectF(40, 40, 120, 120));
    QPainterPath oval;
    oval.addEllipse(QRectF(20, 60, 160, 80));
    QPainterPath int_circle;
    int_circle.addEllipse(QRectF(40, 40, 120, 120)); // Qt draws int QRect(40,40,120,120) as 120x120? see note

    const Draw draw_ellipse = [](QPainter& p) { p.drawEllipse(QRectF(40, 40, 120, 120)); };
    const Draw draw_path = [&](QPainter& p) { p.drawPath(curve_path); };
    const Draw draw_int = [](QPainter& p) { p.drawEllipse(QRect(40, 40, 120, 120)); };
    const Draw draw_oval = [](QPainter& p) { p.drawEllipse(QRectF(20, 60, 160, 80)); };

    QImage sheet(200 * 4, 200 * 3, QImage::Format_ARGB32);
    sheet.fill(Qt::gray);
    QPainter sp(&sheet);
    for (std::size_t t = 0; t < themes.size(); ++t)
    {
        const auto& th = themes[t];
        const QImage e = hand(th.theme, draw_ellipse);
        const QImage pa = hand(th.theme, draw_path);
        const QImage ei = hand(th.theme, draw_int);
        const QImage ov = hand(th.theme, draw_oval);
        const QString n = QString::fromLatin1(th.name);
        out << "== " << th.name << "\n";
        measure("  B7 ellipse QRectF", th.theme, circle, e, plain(th.theme, draw_ellipse), out);
        out << "    center(100,100)=" << e.pixelColor(100, 100).name() << " bg=" << th.theme.background().name()
            << (e.pixelColor(100, 100).rgb() == th.theme.background().rgb() ? " (background)" : " (NOT background)")
            << "\n";
        measure("  B10 path", th.theme, curve_path, pa, plain(th.theme, draw_path), out);
        measure("  ellipse int QRect", th.theme, int_circle, ei, plain(th.theme, draw_int), out);
        measure("  oval 160x80", th.theme, oval, ov, plain(th.theme, draw_oval), out);
        // determinism
        out << "  ellipse repeat identical=" << (hand(th.theme, draw_ellipse) == e ? "yes" : "NO") << "\n";
        sp.drawImage(0, int(t) * 200, e);
        sp.drawImage(200, int(t) * 200, pa);
        sp.drawImage(400, int(t) * 200, ei);
        sp.drawImage(600, int(t) * 200, ov);
        if (t != 1) // keep evidence small: per-theme detail for sketch and chalk, sheet has all
        {
            QImage detail(800, 400, QImage::Format_ARGB32);
            QPainter dp(&detail);
            dp.drawImage(0, 0, enlarge(e, QRect(30, 30, 50, 50), 8));
            dp.drawImage(400, 0, enlarge(pa, QRect(60, 90, 50, 50), 8));
            dp.end();
            detail.save(outdir + "/crop_" + n + ".png");
        }
        if (t == 1)
        {
            QImage detail(800, 400, QImage::Format_ARGB32);
            QPainter dp(&detail);
            dp.drawImage(0, 0, enlarge(e, QRect(30, 30, 50, 50), 8));
            dp.drawImage(400, 0, enlarge(pa, QRect(60, 90, 50, 50), 8));
            dp.end();
            detail.save(outdir + "/crop_" + n + ".png");
        }
        {
            // ellipse closure point (160,100) and the line/curve joint (80,150), sketch row = t*? one file
            QImage joints(800, 400, QImage::Format_ARGB32);
            QPainter jp(&joints);
            jp.drawImage(0, 0, enlarge(e, QRect(140, 80, 40, 40), 10));
            jp.drawImage(400, 0, enlarge(pa, QRect(60, 120, 40, 40), 10));
            jp.end();
            if (t == 0)
                joints.save(outdir + "/joints_sketch.png");
        }
    }
    // merge check (T5 + T6): rect, triangle, polyline, ellipse and cubic path in one drawing, all themes
    {
        const QPolygonF tri{{QPointF(100, 30), QPointF(170, 170), QPointF(30, 170)}};
        const QPolygonF pl{{QPointF(30, 30), QPointF(100, 170), QPointF(170, 30)}};
        const Draw draw_rect = [](QPainter& p) { p.drawRect(QRectF(40, 40, 120, 120)); };
        const Draw draw_tri = [&](QPainter& p) { p.drawPolygon(tri); };
        const Draw draw_pl = [&](QPainter& p) { p.drawPolyline(pl); };
        QPainterPath rect_path;
        rect_path.addRect(QRectF(40, 40, 120, 120));
        QPainterPath tri_path;
        tri_path.addPolygon(tri);
        tri_path.closeSubpath();
        QPainterPath pl_path;
        pl_path.addPolygon(pl);
        QPainterPath all_path;
        all_path.addPath(rect_path);
        all_path.addPath(tri_path);
        all_path.addPath(pl_path);
        all_path.addPath(circle);
        all_path.addPath(curve_path);
        const Draw draw_all = [&](QPainter& p) {
            draw_rect(p); draw_tri(p); draw_pl(p); p.drawEllipse(QRectF(40, 40, 120, 120)); p.drawPath(curve_path);
        };
        QImage merged(200 * 5, 200 * 3, QImage::Format_ARGB32);
        merged.fill(Qt::gray);
        QPainter mp(&merged);
        for (std::size_t t = 0; t < themes.size(); ++t)
        {
            const auto& th = themes[t];
            out << "== merge " << th.name << "\n";
            const QImage r = hand(th.theme, draw_rect);
            const QImage tr = hand(th.theme, draw_tri);
            const QImage p = hand(th.theme, draw_pl);
            const QImage a = hand(th.theme, draw_all);
            measure("  B6 rect", th.theme, rect_path, r, plain(th.theme, draw_rect), out);
            out << "    rect center(100,100) background="
                << (r.pixelColor(100, 100).rgb() == th.theme.background().rgb() ? "yes" : "NO") << "\n";
            measure("  B8 triangle", th.theme, tri_path, tr, plain(th.theme, draw_tri), out);
            measure("  B9 polyline", th.theme, pl_path, p, plain(th.theme, draw_pl), out);
            measure("  all five shapes", th.theme, all_path, a, plain(th.theme, draw_all), out);
            mp.drawImage(0, int(t) * 200, r);
            mp.drawImage(200, int(t) * 200, tr);
            mp.drawImage(400, int(t) * 200, p);
            mp.drawImage(600, int(t) * 200, a);
            mp.drawImage(800, int(t) * 200, plain(th.theme, draw_all));
        }
        mp.end();
        merged.save(outdir + "/merge_sheet_all_themes.png");
    }
    // curiosity: arc and rounded rect, sketch
    const QImage extra = hand(hpq::Theme::sketch(), [](QPainter& p) {
        p.drawArc(QRectF(20, 20, 160, 160), 30 * 16, 120 * 16);
        p.drawRoundedRect(QRectF(40, 90, 120, 80), 20, 20);
    });
    sp.end();
    sheet.save(outdir + "/sheet_all_themes.png");
    extra.scaled(400, 400, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(outdir + "/extra_arc_roundedrect_sketch.png");
    // plain reference for comparison
    plain(hpq::Theme::sketch(), draw_ellipse).save(outdir + "/plain_ellipse_sketch.png");
    out << "done\n";
    return 0;
}
