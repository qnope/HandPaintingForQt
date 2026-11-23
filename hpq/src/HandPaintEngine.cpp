#include <hpq/HandPaintEngine.h>

#include <hpq/FillStyle.h>
#include <hpq/GeometryEngine.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include "BrushStroke.h"
#include "ChalkStroke.h"
#include "ChalkTexture.h"
#include "PathTiles.h"
#include "WatercolorTexture.h"
#include "WatercolorWash.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QPaintDevice>
#include <QPainter>
#include <QPainterPathStroker>
#include <QRegion>
#include <QScrollBar>
#include <QTextItem>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <random>
#include <vector>

namespace hpq {

namespace {

// Width, in device pixels, of the lines of a hatched fill.
constexpr double HachureWidth = 1.0;
// Coordinates are rounded to this fraction of a pixel before they make the seed.
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

// A brush is soft: the line of an outline is this much wider, in pixels, than the pen it
// stands for. A hachure is drawn with the tip of the brush, and keeps the width of its pen.
constexpr double BrushExtraWidth = 1.0;
// A brush ribbon is at most this many times wider than its width.
constexpr double BrushWidestPressure = 2.2;

// Width, in pixels, of the brush line drawn for `pen`; a cosmetic pen is one pixel wide.
double brush_width(const QPen &pen, bool is_outline) {
    return std::max(pen.widthF(), 1.0) + (is_outline ? BrushExtraWidth : 0.0);
}

// A hachure of watercolor is one stroke of a small flat brush: it is this much wider, in
// pixels, than the pen it stands for.
constexpr double WatercolorHachureExtraWidth = 1.0;

// The miter limit of a pen that was given none.
constexpr double DefaultMiterLimit = 2.0;

// A wash does not follow the outline of its shape: it gets another seed than the outline.
constexpr std::uint32_t WashSeedMask = 0x5bd1e995u;

// `brush` with its image moved by `offset`: the image follows what is drawn there.
QBrush moved(QBrush brush, const QPointF &offset) {
    brush.setTransform(brush.transform() * QTransform::fromTranslate(offset.x(), offset.y()));
    return brush;
}

// Reads the scroll offsets of a view of items, which QAbstractItemView keeps protected: a
// pointer to a member, taken through a derived class, calls them on any view. They are in
// pixels, also when the view scrolls by items.
class ItemViewOffsets : public QAbstractItemView {
public:
    static QPointF of(const QAbstractItemView &view) {
        return QPointF((view.*&ItemViewOffsets::horizontalOffset)(), (view.*&ItemViewOffsets::verticalOffset)());
    }
};

// How far the content shown by `device` is scrolled: none, unless it is the viewport of a
// scroll area.
QPointF scroll_offset(const QPaintDevice *device) {
    if (device == nullptr || device->devType() != QInternal::Widget) {
        return QPointF();
    }
    const auto *widget = static_cast<const QWidget *>(device);
    const auto *area = qobject_cast<const QAbstractScrollArea *>(widget->parentWidget());
    if (area == nullptr || area->viewport() != widget) {
        return QPointF();
    }
    if (const auto *view = qobject_cast<const QAbstractItemView *>(area)) {
        return ItemViewOffsets::of(*view);
    }
    return QPointF(area->horizontalScrollBar()->value(), area->verticalScrollBar()->value());
}

// True when `pen` draws a plain line of one color, which chalk or a brush can follow with
// uneven pressure.
bool is_plain_line(const QPen &pen) {
    return pen.style() == Qt::SolidLine && pen.brush().style() == Qt::SolidPattern;
}

}  // namespace

// A shape whose paths are all kept needs no seed: its seed is made only when it is asked for.
class HandPaintEngine::ShapeSeed {
public:
    explicit ShapeSeed(const QPainterPath &shape) : m_shape(shape) {}

    std::uint32_t value() {
        if (!m_value) {
            m_value = path_seed(m_shape);
        }
        return *m_value;
    }

private:
    const QPainterPath &m_shape;
    std::optional<std::uint32_t> m_value;
};

HandPaintEngine::HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                                 PaintCache *cache)
    : QPaintEngine(QPaintEngine::AllFeatures),
      m_target(target),
      m_theme(theme),
      m_geometry_engine(geometry_engine),
      m_cache(cache) {}

bool HandPaintEngine::begin(QPaintDevice *) { return true; }

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
    save_target_in_logical_coordinates();
    m_target.drawPixmap(rect, pixmap, source_rect);
    m_target.restore();
}

void HandPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                Qt::ImageConversionFlags flags) {
    save_target_in_logical_coordinates();
    m_target.drawImage(rect, image, source_rect, flags);
    m_target.restore();
}

void HandPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) {
    save_target_in_logical_coordinates();
    m_target.drawTiledPixmap(rect, pixmap, offset);
    m_target.restore();
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

template <typename Make>
std::vector<QPainterPath> HandPaintEngine::kept_paths(PaintCache::PathKind kind, const QPainterPath &shape,
                                                      const Tool &tool, Make make) {
    if (m_cache == nullptr) {
        return make();
    }
    // The values of the cap styles and of the join styles are bits of their own, above the ribbons.
    const int tool_number = static_cast<int>(tool.ribbon) | static_cast<int>(tool.cap_style) |
                            static_cast<int>(tool.join_style);
    // Without a ribbon the paths are the strokes themselves, whatever the width of their pen.
    const double width = tool.ribbon == Ribbon::None ? 0.0 : tool.width;
    return m_cache->paths(kind, shape, tool_number, width, make);
}

void HandPaintEngine::draw_shape(const QPainterPath &logical_shape, ShapeKind kind) {
    // The shape is made rough in the coordinates of the content, then drawn where the content
    // is on the device: a shape keeps its rough paths when its view scrolls, and two shapes at
    // two places of the content get two seeds.
    m_origin = kind == ShapeKind::Open ? line_origin(logical_shape) : content_origin();
    const QPainterPath shape = m_transform.map(logical_shape).translated(-m_origin);
    const RoughOptions options = m_theme.rough_options();
    ShapeSeed shape_seed(shape);
    const bool filled = m_brush.style() != Qt::NoBrush;
    if (kind != ShapeKind::Open && filled) {
        fill_shape(shape, kind, options, shape_seed);
    }
    if (m_pen.style() != Qt::NoPen) {
        // A rectangle with no area is a line: it frames nothing.
        const bool is_frame = kind == ShapeKind::Rectangle && !filled && !shape.boundingRect().isEmpty();
        const Tool tool = tool_of(m_pen, StrokeKind::Outline);
        const std::vector<QPainterPath> outlines = kept_paths(
            is_frame ? PaintCache::PathKind::FrameOutline : PaintCache::PathKind::Outline, shape, tool, [&] {
                std::vector<QPainterPath> made;
                std::uint32_t stroke_seed = shape_seed.value();
                for (const QPainterPath &outline : m_geometry_engine.outline(shape, options, shape_seed.value())) {
                    // Each stroke of the outline gets its own seed: with chalk, its own pressure and gaps.
                    made.push_back(marked(is_frame ? kept_outside(outline, shape) : outline, tool, ++stroke_seed));
                }
                return made;
            });
        for (const QPainterPath &outline : outlines) {
            draw_marked(outline, m_pen, tool);
        }
    }
}

void HandPaintEngine::fill_shape(const QPainterPath &shape, ShapeKind kind, const RoughOptions &options,
                                 ShapeSeed &shape_seed) {
    const bool is_watercolor = m_theme.medium() == Medium::Watercolor;
    // One area, in a list of one path.
    const QPainterPath area = kept_paths(PaintCache::PathKind::FillArea, shape, Tool(), [&] {
        const std::uint32_t seed = shape_seed.value();
        return std::vector<QPainterPath>{
            m_geometry_engine.fill_area(shape, options, is_watercolor ? seed ^ WashSeedMask : seed)};
    }).front();
    if (is_hatched(m_brush)) {
        const QPen pen(m_brush, HachureWidth, Qt::SolidLine, Qt::RoundCap);
        const Tool tool = tool_of(pen, StrokeKind::Hachure);
        // What the tool marks along the hachures, in a list of one path; none when the shape
        // cannot be hatched.
        const std::vector<QPainterPath> hachures = kept_paths(PaintCache::PathKind::Hachure, shape, tool, [&] {
            const QPainterPath hachure = m_geometry_engine.hachure(shape, options, shape_seed.value());
            return hachure.isEmpty() ? std::vector<QPainterPath>()
                                     : std::vector<QPainterPath>{marked(hachure, tool, shape_seed.value())};
        });
        if (!hachures.empty()) {
            // The paper shows between the hachures, and hides what was drawn under the shape.
            draw_on_target(area, Qt::NoPen, seen_through_medium(m_theme.palette().window()));
            draw_marked(hachures.front(), pen, tool);
            return;
        }
    }
    lay_on_target(area, m_brush, kind == ShapeKind::Rectangle && m_pen.style() == Qt::NoPen);
}

void HandPaintEngine::lay_on_target(const QPainterPath &area, const QBrush &brush, bool is_band) {
    if (!is_wash(brush)) {
        draw_on_target(area, Qt::NoPen, seen_through_medium(brush));
        return;
    }
    save_target_in_device_coordinates();
    // The wash settles from the origin of the painter: moved to the origin of the content, by
    // whole pixels, it scrolls with the content.
    const QPointF origin = m_origin.toPoint();
    m_target.translate(origin);
    paint_wash(m_target, area.translated(m_origin - origin), brush.color(), is_band ? WashKind::Band : WashKind::Patch,
               m_cache);
    m_target.restore();
}

bool HandPaintEngine::is_wash(const QBrush &brush) const {
    return m_theme.medium() == Medium::Watercolor && brush.style() == Qt::SolidPattern && !is_ground(brush.color());
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

HandPaintEngine::Tool HandPaintEngine::tool_of(const QPen &pen, StrokeKind kind) const {
    const Medium medium = m_theme.medium();
    const bool is_outline = kind == StrokeKind::Outline;
    const bool is_chalk = medium == Medium::Chalk;
    const bool is_watercolor = medium == Medium::Watercolor;
    // A brush follows a plain line; a dashed line is drawn by its pen, as a pencil draws it.
    const bool is_brush = (medium == Medium::Ink || is_watercolor) && is_plain_line(pen);
    Tool tool;
    if (is_brush) {
        tool.ribbon = Ribbon::Brush;
    } else if (is_chalk && is_plain_line(pen)) {
        tool.ribbon = Ribbon::Chalk;
    } else if (!is_chalk && m_cache != nullptr && is_plain_line(pen) && !is_thin(pen) &&
               (pen.joinStyle() != Qt::MiterJoin || pen.miterLimit() == DefaultMiterLimit)) {
        // The target turns a wide line into an area each time it strokes it: here the area is
        // made once, and kept with the stroke.
        tool.ribbon = Ribbon::Pen;
        tool.cap_style = pen.capStyle();
        tool.join_style = pen.joinStyle();
    }
    tool.width = is_chalk   ? chalk_width(pen)
                 : is_brush ? brush_width(pen, is_outline) +
                                  (is_watercolor && !is_outline ? WatercolorHachureExtraWidth : 0.0)
                            : pen.widthF();
    // An outline wobbles up to max_offset away from the shape, and its pen is wide: a frame on
    // the edge of a widget would be cut by the widget's area without this margin.
    // Hachures stay inside their shape: they get no margin.
    const double widest = tool.width * (is_chalk ? ChalkWidestPressure : is_brush ? BrushWidestPressure : 1.0);
    tool.system_clip_margin =
        is_outline ? static_cast<int>(std::ceil(m_theme.rough_options().max_offset + widest)) : 0;
    return tool;
}

bool HandPaintEngine::is_thin(const QPen &pen) const {
    if (pen.isCosmetic()) {
        return true;
    }
    const QTransform to_device = m_target.deviceTransform();
    return pen.widthF() * std::hypot(to_device.m11(), to_device.m12()) <= 1.0;
}

QPainterPath HandPaintEngine::marked(const QPainterPath &stroke, const Tool &tool, std::uint32_t stroke_seed) const {
    switch (tool.ribbon) {
    case Ribbon::Pen: {
        QPainterPathStroker stroker;
        stroker.setWidth(tool.width);
        stroker.setCapStyle(tool.cap_style);
        stroker.setJoinStyle(tool.join_style);
        stroker.setMiterLimit(DefaultMiterLimit);
        return stroker.createStroke(stroke);
    }
    case Ribbon::Chalk:
        return chalk_ribbon(stroke, tool.width, stroke_seed);
    case Ribbon::Brush:
        return brush_ribbon(stroke, tool.width, stroke_seed);
    case Ribbon::None:
        break;
    }
    return stroke;
}

void HandPaintEngine::draw_marked(const QPainterPath &marked_stroke, const QPen &pen, const Tool &tool) {
    const Medium medium = m_theme.medium();
    if (tool.ribbon == Ribbon::Brush && medium == Medium::Watercolor) {
        glaze_on_target(marked_stroke, moved(pigment_brush(pen.color(), target_pixel_ratio()), m_origin),
                        tool.system_clip_margin);
    } else if (tool.ribbon == Ribbon::Brush || tool.ribbon == Ribbon::Pen) {
        draw_on_target(marked_stroke, Qt::NoPen, pen.brush(), tool.system_clip_margin);
    } else if (tool.ribbon == Ribbon::Chalk) {
        draw_on_target(marked_stroke, Qt::NoPen,
                       moved(chalk_brush(pen.color(), ChalkGrain::Stroke, target_pixel_ratio()), m_origin),
                       tool.system_clip_margin);
    } else if (medium == Medium::Chalk) {
        // A dashed line keeps its dashes: it only gets the grain.
        QPen grainy_pen = chalk_pen(pen, ChalkGrain::Stroke, m_origin);
        grainy_pen.setWidthF(tool.width);
        draw_on_target(marked_stroke, grainy_pen, Qt::NoBrush, tool.system_clip_margin);
    } else {
        draw_on_target(marked_stroke, pen, Qt::NoBrush, tool.system_clip_margin);
    }
}

QBrush HandPaintEngine::seen_through_medium(const QBrush &brush) const {
    if (brush.style() != Qt::SolidPattern) {
        return brush;
    }
    const QColor color = brush.color();
    switch (m_theme.medium()) {
    case Medium::Chalk:
        // The slate stays where it is; the chalk rubbed on a shape scrolls with the content.
        return is_ground(color) ? slate_brush(color, target_pixel_ratio())
                                : moved(chalk_brush(color, ChalkGrain::Fill, target_pixel_ratio()), m_origin);
    case Medium::Watercolor:
        return is_ground(color) ? paper_brush(color, target_pixel_ratio()) : brush;
    default:
        return brush;
    }
}

// Chalk cannot paint the slate, nor watercolor the white of the paper: a fill of the color of
// the window or of a view is the slate or the paper.
bool HandPaintEngine::is_ground(const QColor &color) const {
    const QPalette palette = m_theme.palette();
    return color == palette.color(QPalette::Active, QPalette::Window) ||
           color == palette.color(QPalette::Active, QPalette::Base);
}

// A pen that already draws a pattern or a gradient is returned unchanged.
QPen HandPaintEngine::chalk_pen(QPen pen, ChalkGrain grain, const QPointF &origin) const {
    if (pen.brush().style() == Qt::SolidPattern) {
        pen.setBrush(moved(chalk_brush(pen.color(), grain, target_pixel_ratio()), origin));
    }
    return pen;
}

// The state of the device painter maps its logical coordinates to the device. Its combined
// transform maps them to the widget being painted: the two differ by where that widget is on
// the device.
QPointF HandPaintEngine::widget_origin() const {
    return m_transform.map(QPointF()) - painter()->combinedTransform().map(QPointF());
}

QPointF HandPaintEngine::content_origin() const {
    return widget_origin() - scroll_offset(painter()->device());
}

QPointF HandPaintEngine::line_origin(const QPainterPath &logical_line) const {
    const QPaintDevice *widget = painter()->device();
    const QRectF line = painter()->combinedTransform().map(logical_line).boundingRect();
    QPointF origin = content_origin();
    // A pen of one pixel draws through the middle of the pixels: half a pixel from the edges.
    if (line.height() == 0.0 && line.left() <= 1.0 && line.right() >= widget->width() - 1.0) {
        origin.setX(widget_origin().x());
    }
    if (line.width() == 0.0 && line.top() <= 1.0 && line.bottom() >= widget->height() - 1.0) {
        origin.setY(widget_origin().y());
    }
    return origin;
}

double HandPaintEngine::target_pixel_ratio() const {
    return m_target.device()->devicePixelRatioF();
}

void HandPaintEngine::draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush,
                                     int system_clip_margin) {
    save_target_in_device_coordinates(system_clip_margin);
    m_target.setPen(pen);
    m_target.setBrush(brush);
    m_target.drawPath(path.translated(m_origin));
    m_target.restore();
}

void HandPaintEngine::glaze_on_target(const QPainterPath &area, const QBrush &brush, int system_clip_margin) {
    save_target_in_device_coordinates(system_clip_margin);
    m_target.setRenderHint(QPainter::Antialiasing);
    if (m_target.paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
        m_target.setCompositionMode(QPainter::CompositionMode_Darken);
    }
    m_target.setPen(Qt::NoPen);
    m_target.setBrush(brush);
    const QPainterPath glaze = area.translated(m_origin);
    // The raster engine of Qt smooths a path in bands, and goes over the whole path for each
    // band: a long line takes as many passes as it is long. Cut in tiles, it takes one pass a
    // tile. Another engine might smooth each tile on its own, and show the cuts.
    if (m_target.paintEngine()->type() == QPaintEngine::Raster) {
        for (const QPainterPath &piece : path_tiles(glaze, m_target.deviceTransform())) {
            m_target.drawPath(piece);
        }
    } else {
        m_target.drawPath(glaze);
    }
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

namespace {

constexpr double PointsPerInch = 72.0;

// Pixel size of `font` on `device`: its pixel size, or its point size converted with the device DPI.
int pixel_size(const QFont &font, const QPaintDevice &device) {
    if (font.pixelSize() > 0) {
        return font.pixelSize();
    }
    return static_cast<int>(std::lround(font.pointSizeF() * device.logicalDpiY() / PointsPerInch));
}

}  // namespace

// Qt writes the text of a selection in the HighlightedText color of the color group of its widget.
bool HandPaintEngine::shows_text(const QColor &color) const {
    if (m_theme.text_ink() == TextInk::Visible) {
        return true;
    }
    const QPalette palette = m_theme.palette();
    return color == palette.color(QPalette::Active, QPalette::HighlightedText) ||
           color == palette.color(QPalette::Inactive, QPalette::HighlightedText) ||
           color == palette.color(QPalette::Disabled, QPalette::HighlightedText);
}

void HandPaintEngine::drawTextItem(const QPointF &position, const QTextItem &text_item) {
    if (!shows_text(m_pen.color())) {
        return;
    }
    QFont font = m_theme.font();
    font.setPixelSize(pixel_size(text_item.font(), *paintDevice()));
    save_target_in_logical_coordinates();
    m_target.setPen(m_theme.medium() == Medium::Chalk ? chalk_pen(m_pen, ChalkGrain::Text, content_origin()) : m_pen);
    m_target.setFont(font);
    m_target.drawText(position, text_item.text());
    m_target.restore();
}

}  // namespace hpq
