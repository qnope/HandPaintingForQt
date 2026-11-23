#include <hpq/HandPaintEngine.h>

#include <hpq/FillStyle.h>
#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include "ChalkStroke.h"
#include "ChalkTexture.h"

#include <QPaintDevice>
#include <QPainter>
#include <QRegion>
#include <QTextItem>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace hpq {

namespace {

// Width, in device pixels, of the lines of a hatched fill.
constexpr double HachureWidth = 1.0;
// Coordinates are rounded to this fraction of a device pixel before they make the seed.
constexpr double SeedPrecision = 100.0;

// `region` with each of its rectangles grown by `margin` on every side.
QRegion grown(const QRegion &region, int margin) {
    if (margin == 0) {
        return region;
    }
    QRegion result;
    for (const QRect &rect : region) {
        result += rect.adjusted(-margin, -margin, margin, margin);
    }
    return result;
}

// `outline` kept out of `frame`: it follows the edge of `frame` where it would enter it.
QPainterPath kept_outside(QPainterPath outline, const QPainterPath &frame) {
    outline.setFillRule(frame.fillRule());
    return outline.united(frame);
}

std::uint32_t rounded(double coordinate) {
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::llround(coordinate * SeedPrecision)));
}

// A seed that std::seed_seq makes from the element types and rounded coordinates of `path`.
std::uint32_t path_seed(const QPainterPath &path) {
    std::vector<std::uint32_t> values;
    values.reserve(3 * static_cast<std::size_t>(path.elementCount()));
    for (int index = 0; index < path.elementCount(); ++index) {
        const QPainterPath::Element element = path.elementAt(index);
        values.push_back(static_cast<std::uint32_t>(element.type));
        values.push_back(rounded(element.x));
        values.push_back(rounded(element.y));
    }
    std::seed_seq seeds(values.begin(), values.end());
    std::uint32_t seed = 0;
    seeds.generate(&seed, &seed + 1);
    return seed;
}

// Chalk is blunt: its line is this much wider, in pixels, than the pen it stands for.
constexpr double ChalkExtraWidth = 1.2;
// A chalk ribbon is at most this many times wider than its width.
constexpr double ChalkWidestPressure = 2.0;

// Width, in pixels, of the chalk line drawn for `pen`; a cosmetic pen is one pixel wide.
double chalk_width(const QPen &pen) {
    return std::max(pen.widthF(), 1.0) + ChalkExtraWidth;
}

// True when `pen` draws a plain line of one color, which chalk can follow with uneven pressure.
bool is_plain_line(const QPen &pen) {
    return pen.style() == Qt::SolidLine && pen.brush().style() == Qt::SolidPattern;
}

}  // namespace

HandPaintEngine::HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                                 PathCache *path_cache)
    : QPaintEngine(QPaintEngine::AllFeatures),
      m_target(target),
      m_theme(theme),
      m_geometry_engine(geometry_engine),
      m_path_cache(path_cache) {}

namespace {

// The bounds of what `painter` can paint, in its coordinates: its clip and the system clip
// of its device (the area that Qt repaints). None when nothing limits it.
std::optional<QRectF> sight_of(const QPainter &painter) {
    std::optional<QRectF> sight;
    const QRegion system_clip = painter.paintEngine()->systemClip();
    const QTransform to_device = painter.deviceTransform();
    if (!system_clip.isEmpty() && to_device.isInvertible()) {
        sight = to_device.inverted().mapRect(QRectF(system_clip.boundingRect()));
    }
    if (painter.hasClipping()) {
        const QRectF clip = painter.clipBoundingRect();
        sight = sight ? sight->intersected(clip) : clip;
    }
    return sight;
}

}  // namespace

bool HandPaintEngine::begin(QPaintDevice *) {
    m_target_sight = sight_of(m_target);
    const QTransform to_device = m_target.deviceTransform();
    m_target_scale = std::hypot(to_device.m11(), to_device.m12());
    return true;
}

bool HandPaintEngine::is_out_of_sight(const QRectF &bounds, double margin) const {
    const QRectF reach = bounds.adjusted(-margin, -margin, margin, margin);
    // QRectF::intersects() is false for an empty rectangle: a line has an empty bounding rectangle.
    const auto misses = [&reach](const QRectF &area) {
        return reach.right() < area.left() || reach.left() > area.right() || reach.bottom() < area.top() ||
               reach.top() > area.bottom();
    };
    if (m_target_sight && misses(*m_target_sight)) {
        return true;
    }
    const QRegion system_clip = systemClip();
    return !system_clip.isEmpty() && misses(QRectF(system_clip.boundingRect()));
}

double HandPaintEngine::outline_overflow() const {
    const bool is_chalk = m_theme.medium() == Medium::Chalk;
    const double widest = is_chalk ? chalk_width(m_pen) * ChalkWidestPressure : m_pen.widthF();
    return std::ceil(m_theme.rough_options().max_offset + widest) + 1.0;
}

bool HandPaintEngine::end() { return true; }

void HandPaintEngine::updateState(const QPaintEngineState &state) {
    const DirtyFlags flags = state.state();
    if (flags & DirtyPen) {
        m_pen = state.pen();
    }
    if (flags & DirtyBrush) {
        m_brush = state.brush();
    }
    if (flags & DirtyTransform) {
        m_transform = state.transform();
    }
    if (flags & (DirtyClipPath | DirtyClipRegion | DirtyClipEnabled)) {
        // QPainter::clipPath() is in the current logical coordinates: map it back to the device
        // with the engine's transform. Unlike QPainter::combinedTransform(), it includes the
        // redirection offset of a child widget rendered through a shared painter.
        const QPainter &painter = *state.painter();
        m_clipping = painter.hasClipping();
        m_clip = m_clipping ? state.transform().map(painter.clipPath()) : QPainterPath();
    }
}

void HandPaintEngine::drawPath(const QPainterPath &path) { draw_shape(path, ShapeKind::Closed); }

void HandPaintEngine::drawRects(const QRectF *rects, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape;
        shape.addRect(rects[index]);
        draw_shape(shape, ShapeKind::Rectangle);
    }
}

void HandPaintEngine::drawLines(const QLineF *lines, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath shape(lines[index].p1());
        shape.lineTo(lines[index].p2());
        draw_shape(shape, ShapeKind::Open);
    }
}

void HandPaintEngine::drawEllipse(const QRectF &rect) {
    QPainterPath shape;
    shape.addEllipse(rect);
    draw_shape(shape, ShapeKind::Closed);
}

// A polyline is an open shape; the other modes are closed and keep their fill rule.
void HandPaintEngine::drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) {
    if (count <= 0) {
        return;
    }
    QPainterPath shape(points[0]);
    for (int index = 1; index < count; ++index) {
        shape.lineTo(points[index]);
    }
    const bool closed = mode != PolylineMode;
    if (closed) {
        shape.closeSubpath();
    }
    shape.setFillRule(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    draw_shape(shape, closed ? ShapeKind::Closed : ShapeKind::Open);
}

// Images reach the target unchanged.
void HandPaintEngine::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) {
    if (is_out_of_sight(m_transform.mapRect(rect), 1.0)) {
        return;
    }
    save_target_in_logical_coordinates();
    m_target.drawPixmap(rect, pixmap, source_rect);
    m_target.restore();
}

void HandPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                Qt::ImageConversionFlags flags) {
    if (is_out_of_sight(m_transform.mapRect(rect), 1.0)) {
        return;
    }
    save_target_in_logical_coordinates();
    m_target.drawImage(rect, image, source_rect, flags);
    m_target.restore();
}

void HandPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) {
    if (is_out_of_sight(m_transform.mapRect(rect), 1.0)) {
        return;
    }
    save_target_in_logical_coordinates();
    m_target.drawTiledPixmap(rect, pixmap, offset);
    m_target.restore();
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

template <typename Make>
std::vector<QPainterPath> HandPaintEngine::kept_paths(PathCache::Kind kind, const QPainterPath &shape, Ink ink,
                                                      const QPen &pen, Make make) {
    if (!m_path_cache) {
        return make();
    }
    // What the paths depend on besides the shape: the width of a ribbon, or the width, the
    // caps and the joins of a stroked area.
    double width = 0.0;
    int variant = 0;
    if (ink == Ink::ChalkRibbon) {
        width = chalk_width(pen);
    } else if (ink == Ink::StrokedArea) {
        width = pen.widthF();
        variant = 1 + static_cast<int>(pen.capStyle()) + static_cast<int>(pen.joinStyle());
    }
    return m_path_cache->paths(kind, shape, width, variant, make);
}

void HandPaintEngine::draw_shape(const QPainterPath &logical_shape, ShapeKind kind) {
    const QPainterPath shape = m_transform.map(logical_shape);
    // What cannot show is not drawn: its rough paths are not even made.
    if (is_out_of_sight(shape.controlPointRect(), outline_overflow())) {
        return;
    }
    const RoughOptions options = m_theme.rough_options();
    const bool filled = m_brush.style() != Qt::NoBrush;
    if (kind != ShapeKind::Open && filled) {
        fill_shape(shape, options);
    }
    if (m_pen.style() != Qt::NoPen) {
        // A rectangle with no area is a line: it frames nothing.
        const bool is_frame = kind == ShapeKind::Rectangle && !filled && !shape.boundingRect().isEmpty();
        const Ink ink = ink_of(m_pen);
        const std::vector<QPainterPath> strokes =
            kept_paths(is_frame ? PathCache::Kind::FrameOutline : PathCache::Kind::Outline, shape, ink, m_pen, [&] {
                const std::uint32_t shape_seed = seed(shape);
                std::vector<QPainterPath> inked_strokes;
                std::uint32_t stroke_seed = shape_seed;
                for (const QPainterPath &outline : m_geometry_engine.outline(shape, options, shape_seed)) {
                    // Each stroke of the outline gets its own seed: with chalk, its own pressure and gaps.
                    inked_strokes.push_back(
                        inked(is_frame ? kept_outside(outline, shape) : outline, ink, m_pen, ++stroke_seed));
                }
                return inked_strokes;
            });
        for (const QPainterPath &stroke : strokes) {
            draw_stroke(stroke, m_pen, StrokeKind::Outline);
        }
    }
}

void HandPaintEngine::fill_shape(const QPainterPath &shape, const RoughOptions &options) {
    const QPainterPath area = kept_paths(PathCache::Kind::FillArea, shape, Ink::Line, QPen(), [&] {
        return std::vector<QPainterPath>{m_geometry_engine.fill_area(shape, options, seed(shape))};
    }).front();
    const QPen hachure_pen(m_brush, HachureWidth, Qt::SolidLine, Qt::RoundCap);
    QPainterPath hachure;
    if (is_hatched(m_brush)) {
        const Ink ink = ink_of(hachure_pen);
        hachure = kept_paths(PathCache::Kind::Hachure, shape, ink, hachure_pen, [&] {
            const std::uint32_t shape_seed = seed(shape);
            const QPainterPath lines = m_geometry_engine.hachure(shape, options, shape_seed);
            // No lines: the shape cannot be hatched, and no ribbon stands for that.
            return std::vector<QPainterPath>{lines.isEmpty() ? lines : inked(lines, ink, hachure_pen, shape_seed)};
        }).front();
    }
    if (hachure.isEmpty()) {
        draw_on_target(area, Qt::NoPen, seen_through_medium(m_brush));
        return;
    }
    // The paper shows between the hachures, and hides what was drawn under the shape.
    draw_on_target(area, Qt::NoPen, seen_through_medium(m_theme.palette().window()));
    draw_stroke(hachure, hachure_pen, StrokeKind::Hachure);
}

bool HandPaintEngine::is_hatched(const QBrush &brush) const {
    if (m_theme.fill_style() != FillStyle::Hachure) {
        return false;
    }
    // A brush that lets the drawing under it show is a tint: the paper must not hide that drawing.
    if (!brush.isOpaque()) {
        return false;
    }
    // Hachures in the color of the paper would not show.
    const QBrush paper = m_theme.palette().window();
    return brush.style() != Qt::SolidPattern || brush.color() != paper.color();
}

HandPaintEngine::Ink HandPaintEngine::ink_of(const QPen &pen) const {
    if (!is_plain_line(pen)) {
        return Ink::Line;
    }
    if (m_theme.medium() == Medium::Chalk) {
        return Ink::ChalkRibbon;
    }
    // The target strokes a line of one device pixel fast. A wider one, it turns into an area at
    // every repaint: with a path cache, that area is made once and kept.
    const bool is_thin = pen.isCosmetic() || pen.widthF() * m_target_scale <= 1.0;
    return m_path_cache && !is_thin ? Ink::StrokedArea : Ink::Line;
}

QPainterPath HandPaintEngine::inked(const QPainterPath &stroke, Ink ink, const QPen &pen,
                                    std::uint32_t stroke_seed) const {
    switch (ink) {
    case Ink::ChalkRibbon:
        return chalk_ribbon(stroke, chalk_width(pen), stroke_seed);
    case Ink::StrokedArea: {
        QPainterPathStroker stroker;
        stroker.setWidth(pen.widthF());
        stroker.setCapStyle(pen.capStyle());
        stroker.setJoinStyle(pen.joinStyle());
        stroker.setMiterLimit(pen.miterLimit());
        return stroker.createStroke(stroke);
    }
    default:
        return stroke;
    }
}

void HandPaintEngine::draw_stroke(const QPainterPath &inked_stroke, const QPen &pen, StrokeKind kind) {
    const bool is_chalk = m_theme.medium() == Medium::Chalk;
    const double width = is_chalk ? chalk_width(pen) : pen.widthF();
    // An outline wobbles up to max_offset away from the shape, and its pen is wide: a frame on
    // the edge of a widget would be cut by the widget's area without this margin.
    // Hachures stay inside their shape: they get no margin.
    const double widest = is_chalk ? width * ChalkWidestPressure : width;
    const int system_clip_margin =
        kind == StrokeKind::Outline ? static_cast<int>(std::ceil(m_theme.rough_options().max_offset + widest)) : 0;
    switch (ink_of(pen)) {
    case Ink::ChalkRibbon:
        draw_on_target(inked_stroke, Qt::NoPen, chalk_brush(pen.color(), ChalkGrain::Stroke, target_pixel_ratio()),
                       system_clip_margin);
        return;
    case Ink::StrokedArea:
        draw_on_target(inked_stroke, Qt::NoPen, pen.brush(), system_clip_margin);
        return;
    default:
        break;
    }
    if (!is_chalk) {
        draw_on_target(inked_stroke, pen, Qt::NoBrush, system_clip_margin);
    } else {
        // A dashed line keeps its dashes: it only gets the grain.
        QPen grainy_pen = chalk_pen(pen, ChalkGrain::Stroke);
        grainy_pen.setWidthF(width);
        draw_on_target(inked_stroke, grainy_pen, Qt::NoBrush, system_clip_margin);
    }
}

QBrush HandPaintEngine::seen_through_medium(const QBrush &brush) const {
    if (m_theme.medium() != Medium::Chalk || brush.style() != Qt::SolidPattern) {
        return brush;
    }
    const QColor color = brush.color();
    return is_slate(color) ? slate_brush(color, target_pixel_ratio())
                           : chalk_brush(color, ChalkGrain::Fill, target_pixel_ratio());
}

// Chalk cannot paint the slate: a fill of the color of the window or of a view is the slate.
bool HandPaintEngine::is_slate(const QColor &color) const {
    const QPalette palette = m_theme.palette();
    return color == palette.color(QPalette::Active, QPalette::Window) ||
           color == palette.color(QPalette::Active, QPalette::Base);
}

// A pen that already draws a pattern or a gradient is returned unchanged.
QPen HandPaintEngine::chalk_pen(QPen pen, ChalkGrain grain) const {
    if (pen.brush().style() == Qt::SolidPattern) {
        pen.setBrush(chalk_brush(pen.color(), grain, target_pixel_ratio()));
    }
    return pen;
}

double HandPaintEngine::target_pixel_ratio() const {
    return m_target.device()->devicePixelRatioF();
}

void HandPaintEngine::draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush,
                                     int system_clip_margin) {
    save_target_in_device_coordinates(system_clip_margin);
    m_target.setPen(pen);
    m_target.setBrush(brush);
    m_target.drawPath(path);
    m_target.restore();
}

void HandPaintEngine::save_target_in_device_coordinates(int system_clip_margin) {
    m_target.save();
    // Intersect keeps the clip the target painter already has.
    // The system clip is the area of the widget Qt is rendering through a shared painter:
    // styles draw past it (for example the fill of a selected tab) and rely on it to cut.
    const QRegion system_clip = systemClip();
    if (!system_clip.isEmpty()) {
        m_target.setClipRegion(grown(system_clip, system_clip_margin), Qt::IntersectClip);
    }
    if (m_clipping) {
        m_target.setClipPath(m_clip, Qt::IntersectClip);
    }
}

void HandPaintEngine::save_target_in_logical_coordinates() {
    save_target_in_device_coordinates();
    // Combine keeps the transform the target painter already has.
    m_target.setTransform(m_transform, true);
}

// `shape` is already in device coordinates: the same primitive drawn at the same place on the
// device gets the same seed, and so the same rough paths.
std::uint32_t HandPaintEngine::seed(const QPainterPath &shape) const { return path_seed(shape); }

namespace {

constexpr double PointsPerInch = 72.0;
// No character is drawn wider than this many pixel sizes of its font, nor farther than these
// above and below its base line.
constexpr double MaxCharacterWidths = 2.0;
constexpr double MaxLineHeightsAbove = 2.0;
constexpr double MaxLineHeightsBelow = 1.0;

// Pixel size of `font` on `device`: its pixel size, or its point size converted with the device DPI.
int pixel_size(const QFont &font, const QPaintDevice &device) {
    if (font.pixelSize() > 0) {
        return font.pixelSize();
    }
    return static_cast<int>(std::lround(font.pointSizeF() * device.logicalDpiY() / PointsPerInch));
}

}  // namespace

void HandPaintEngine::drawTextItem(const QPointF &position, const QTextItem &text_item) {
    QFont font = m_theme.font();
    font.setPixelSize(pixel_size(text_item.font(), *paintDevice()));
    // The exact bounds of the text in the theme font would cost its layout, as much as drawing
    // it: these are generous ones, from the size of the font and the number of characters.
    const double size = font.pixelSize();
    const QRectF generous_bounds(position.x() - size, position.y() - MaxLineHeightsAbove * size,
                                 (MaxCharacterWidths * static_cast<double>(text_item.text().size()) + 2.0) * size,
                                 (MaxLineHeightsAbove + MaxLineHeightsBelow) * size);
    if (is_out_of_sight(m_transform.mapRect(generous_bounds), 1.0)) {
        return;
    }
    save_target_in_logical_coordinates();
    m_target.setPen(m_theme.medium() == Medium::Chalk ? chalk_pen(m_pen, ChalkGrain::Text) : m_pen);
    m_target.setFont(font);
    m_target.drawText(position, text_item.text());
    m_target.restore();
}

}  // namespace hpq
