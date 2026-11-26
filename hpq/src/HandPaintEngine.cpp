#include "HandPaintEngine.h"

#include "ChalkTexture.h"
#include "ContentOrigin.h"
#include "SavedPainter.h"
#include "Target.h"
#include "WatercolorTexture.h"
#include "WatercolorWash.h"

#include <hpq/AntialiasingMode.h>
#include <hpq/FillStyle.h>
#include <hpq/GeometryEngine.h>
#include <hpq/Medium.h>
#include <hpq/RoughOptions.h>
#include <hpq/TextInk.h>
#include <hpq/Theme.h>

#include <QPaintDevice>
#include <QPainter>
#include <QRegion>
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
// Smoothing darkens the pixels that a path only touches: up to this many pixels past it.
constexpr double SmoothingReach = 1.0;
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

// A wash does not follow the outline of its shape: it gets another seed than the outline.
constexpr std::uint32_t WashSeedMask = 0x5bd1e995u;

// `brush` of the device painter, as it is laid on the device: a gradient given in the logical
// coordinates of the painter follows its transform and its brush origin, as Qt places it.
QBrush on_device(QBrush brush, const QTransform &transform, const QPointF &brush_origin) {
    const QGradient *gradient = brush.gradient();
    if (gradient != nullptr && gradient->coordinateMode() == QGradient::LogicalMode) {
        brush.setTransform(brush.transform() * QTransform::fromTranslate(brush_origin.x(), brush_origin.y()) *
                           transform);
    }
    return brush;
}

// `pen` of the device painter, as it strokes on the device: as wide as the transform makes it,
// unless it is cosmetic, with its brush on the device.
QPen on_device(QPen pen, const QTransform &transform, const QPointF &brush_origin) {
    if (!pen.isCosmetic()) {
        // How much the transform scales a length, whatever its direction.
        pen.setWidthF(pen.widthF() * std::sqrt(std::abs(transform.determinant())));
    }
    pen.setBrush(on_device(pen.brush(), transform, brush_origin));
    return pen;
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

void HandPaintEngine::updateState(const QPaintEngineState &new_state) {
    const DirtyFlags flags = new_state.state();
    if (flags & DirtyPen) {
        m_pen = new_state.pen();
    }
    if (flags & DirtyBrush) {
        m_brush = new_state.brush();
    }
    if (flags & DirtyTransform) {
        m_transform = new_state.transform();
    }
    if (flags & (DirtyClipPath | DirtyClipRegion | DirtyClipEnabled)) {
        // QPainter::clipPath() is in the current logical coordinates: map it back to the device
        // with the engine's transform. Unlike QPainter::combinedTransform(), it includes the
        // redirection offset of a child widget rendered through a shared painter.
        const QPainter &device_painter = *new_state.painter();
        m_clip = device_painter.hasClipping()
                     ? std::optional<QPainterPath>(new_state.transform().map(device_painter.clipPath()))
                     : std::nullopt;
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
    draw_unchanged(rect, [&] { m_target.drawPixmap(rect, pixmap, source_rect); });
}

void HandPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                Qt::ImageConversionFlags flags) {
    draw_unchanged(rect, [&] { m_target.drawImage(rect, image, source_rect, flags); });
}

void HandPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) {
    draw_unchanged(rect, [&] { m_target.drawTiledPixmap(rect, pixmap, offset); });
}

template <typename Draw>
void HandPaintEngine::draw_unchanged(const QRectF &logical_bounds, Draw draw) {
    if (is_out_of_sight(m_transform.mapRect(logical_bounds), SmoothingReach)) {
        return;
    }
    const SavedPainter saved(m_target);
    follow_device_painter_in_logical_coordinates();
    draw();
}

QPaintEngine::Type HandPaintEngine::type() const { return QPaintEngine::User; }

template <typename Make>
std::vector<QPainterPath> HandPaintEngine::kept_paths(PaintCache::PathKind kind, const QPainterPath &shape,
                                                      const StrokeTool &tool, Make make) {
    if (m_cache == nullptr) {
        return make();
    }
    // Without a ribbon the paths are the strokes themselves, whatever the width of their pen.
    const double width = tool.ribbon == Ribbon::None ? 0.0 : tool.width;
    return m_cache->paths(kind, shape, cache_number(tool), width, make);
}

void HandPaintEngine::draw_shape(const QPainterPath &logical_shape, ShapeKind kind) {
    const bool filled = m_brush.style() != Qt::NoBrush;
    const bool outlined = m_pen.style() != Qt::NoPen;
    const QPen pen = outlined ? on_device(m_pen, m_transform, painter()->brushOrigin()) : QPen(Qt::NoPen);
    // The tool of the outline; none for a shape without outline.
    const std::optional<StrokeTool> outline_tool =
        outlined ? std::optional<StrokeTool>(stroke_tool(pen, StrokeKind::Outline, m_theme, m_target)) : std::nullopt;
    // A shape that cannot show is not drawn: its rough paths are not even looked for.
    if (is_out_of_sight(m_transform.mapRect(logical_shape.controlPointRect()),
                        ink_reach(outline_tool, filled && kind != ShapeKind::Open))) {
        return;
    }
    // The shape is made rough in the coordinates of the content, then drawn where the content
    // is on the device: a shape keeps its rough paths when its view scrolls, and two shapes at
    // two places of the content get two seeds.
    m_origin = kind == ShapeKind::Open ? line_origin(*painter(), m_transform, logical_shape)
                                       : content_origin(*painter(), m_transform);
    const QPainterPath shape = m_transform.map(logical_shape).translated(-m_origin);
    const RoughOptions options = m_theme.rough_options();
    ShapeSeed shape_seed(shape);
    if (kind != ShapeKind::Open && filled) {
        fill_shape(shape, kind, options, shape_seed);
    }
    if (outline_tool) {
        const StrokeTool &tool = *outline_tool;
        // A rectangle with no area is a line: it frames nothing.
        const bool is_frame = kind == ShapeKind::Rectangle && !filled && !shape.boundingRect().isEmpty();
        const std::vector<QPainterPath> outlines = kept_paths(
            is_frame ? PaintCache::PathKind::FrameOutline : PaintCache::PathKind::Outline, shape, tool, [&] {
                std::vector<QPainterPath> made;
                std::uint32_t stroke_seed = shape_seed.value();
                const std::vector<QPainterPath> strokes =
                    is_frame ? m_geometry_engine.frame_outline(shape, options, shape_seed.value())
                             : m_geometry_engine.outline(shape, options, shape_seed.value());
                for (const QPainterPath &outline : strokes) {
                    // Each stroke of the outline gets its own seed: with chalk, its own pressure and gaps.
                    made.push_back(marked(outline, tool, ++stroke_seed));
                }
                return made;
            });
        for (const QPainterPath &outline : outlines) {
            draw_marked(outline, pen, tool);
        }
    }
}

void HandPaintEngine::fill_shape(const QPainterPath &shape, ShapeKind kind, const RoughOptions &options,
                                 ShapeSeed &shape_seed) {
    const bool is_watercolor = m_theme.medium() == Medium::Watercolor;
    // One area, in a list of one path.
    const QPainterPath area = kept_paths(PaintCache::PathKind::FillArea, shape, StrokeTool(), [&] {
        const std::uint32_t seed = shape_seed.value();
        return std::vector<QPainterPath>{
            m_geometry_engine.fill_area(shape, options, is_watercolor ? seed ^ WashSeedMask : seed)};
    }).front();
    const QBrush brush = on_device(m_brush, m_transform, painter()->brushOrigin());
    if (is_hatched(brush)) {
        const QPen pen(brush, HachureWidth, Qt::SolidLine, Qt::RoundCap);
        const StrokeTool tool = stroke_tool(pen, StrokeKind::Hachure, m_theme, m_target);
        // What the tool marks along the hachures, in a list of one path; none when the shape
        // cannot be hatched.
        const std::vector<QPainterPath> hachures = kept_paths(PaintCache::PathKind::Hachure, shape, tool, [&] {
            const QPainterPath hachure = m_geometry_engine.hachure(shape, options, shape_seed.value());
            return hachure.isEmpty() ? std::vector<QPainterPath>()
                                     : std::vector<QPainterPath>{marked(hachure, tool, shape_seed.value())};
        });
        if (!hachures.empty()) {
            // The paper shows between the hachures, and hides what was drawn under the shape.
            draw_on_target(area, Qt::NoPen, seen_through_medium(m_theme.palette().window()), Tiling::Area);
            draw_marked(hachures.front(), pen, tool);
            return;
        }
    }
    lay_on_target(area, brush, kind == ShapeKind::Rectangle && m_pen.style() == Qt::NoPen);
}

void HandPaintEngine::lay_on_target(const QPainterPath &area, const QBrush &brush, bool is_band) {
    if (!is_wash(brush)) {
        draw_on_target(area, Qt::NoPen, seen_through_medium(brush), Tiling::Area);
        return;
    }
    const SavedPainter saved(m_target);
    follow_device_painter();
    // The wash settles from the origin of the painter: moved to the origin of the content, by
    // whole pixels, it scrolls with the content.
    const QPointF origin = m_origin.toPoint();
    m_target.translate(origin);
    paint_wash(m_target, area.translated(m_origin - origin), brush.color(), is_band ? WashKind::Band : WashKind::Patch,
               m_cache);
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

double HandPaintEngine::ink_reach(const std::optional<StrokeTool> &outline_tool, bool filled) const {
    const RoughOptions options = m_theme.rough_options();
    // How far past their rough paths the strokes and the fill put ink.
    double spread = outline_tool ? outline_tool->widest : 0.0;
    if (filled) {
        // The ends of a hachure move along it by less than a gap. A wash spreads past its area.
        if (is_hatched(m_brush)) {
            const StrokeTool hachure_tool =
                stroke_tool(QPen(m_brush, HachureWidth), StrokeKind::Hachure, m_theme, m_target);
            spread = std::max(spread, options.hachure_gap + hachure_tool.widest);
        }
        if (is_wash(m_brush)) {
            spread = std::max(spread, wash_reach());
        }
    }
    return options.max_offset + spread + SmoothingReach;
}

bool HandPaintEngine::is_out_of_sight(const QRectF &bounds, double margin) const {
    return paintable_pixels(m_target, bounds.adjusted(-margin, -margin, margin, margin)).isEmpty();
}

bool HandPaintEngine::is_smoothed() const {
    switch (m_theme.antialiasing_mode()) {
    case AntialiasingMode::Never:
        return false;
    case AntialiasingMode::OnlyWhenAsked:
        return painter()->testRenderHint(QPainter::Antialiasing);
    case AntialiasingMode::Always:
        break;
    }
    return true;
}

void HandPaintEngine::draw_marked(const QPainterPath &marked_stroke, const QPen &pen, const StrokeTool &tool) {
    const Medium medium = m_theme.medium();
    if (tool.ribbon == Ribbon::Brush && medium == Medium::Watercolor) {
        glaze_on_target(marked_stroke, moved(pigment_brush(pen.color(), target_pixel_ratio()), m_origin),
                        tool.system_clip_margin);
    } else if (tool.ribbon == Ribbon::Brush || tool.ribbon == Ribbon::Pen) {
        draw_on_target(marked_stroke, Qt::NoPen, pen.brush(), Tiling::Stroke, tool.system_clip_margin);
    } else if (tool.ribbon == Ribbon::Chalk) {
        draw_on_target(marked_stroke, Qt::NoPen,
                       moved(chalk_brush(pen.color(), ChalkGrain::Stroke, target_pixel_ratio()), m_origin),
                       Tiling::Stroke, tool.system_clip_margin);
    } else if (medium == Medium::Chalk) {
        // A dashed line keeps its dashes: it only gets the grain.
        QPen grainy_pen = chalk_pen(pen, ChalkGrain::Stroke, m_origin);
        grainy_pen.setWidthF(tool.width);
        draw_on_target(marked_stroke, grainy_pen, Qt::NoBrush, Tiling::Stroke, tool.system_clip_margin);
    } else {
        draw_on_target(marked_stroke, pen, Qt::NoBrush, Tiling::Stroke, tool.system_clip_margin);
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

double HandPaintEngine::target_pixel_ratio() const {
    return m_target.device()->devicePixelRatioF();
}

void HandPaintEngine::draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush, Tiling tiling,
                                     int system_clip_margin) {
    const SavedPainter saved(m_target);
    follow_device_painter(system_clip_margin);
    const bool smoothed = is_smoothed();
    m_target.setRenderHint(QPainter::Antialiasing, smoothed);
    if (pen.style() == Qt::NoPen && smoothed) {
        fill_in_pieces(m_target, path, brush, tiling, m_origin, m_cache);
    } else {
        set_to_content(m_target, path, pen, brush, m_origin);
        m_target.drawPath(path);
    }
}

void HandPaintEngine::glaze_on_target(const QPainterPath &area, const QBrush &brush, int system_clip_margin) {
    const SavedPainter saved(m_target);
    follow_device_painter(system_clip_margin);
    if (m_target.paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
        m_target.setCompositionMode(QPainter::CompositionMode_Darken);
    }
    // The smooth edge of watercolor is the one of its medium: lost only when the theme never smooths.
    if (m_theme.antialiasing_mode() != AntialiasingMode::Never) {
        m_target.setRenderHint(QPainter::Antialiasing);
        fill_in_pieces(m_target, area, brush, Tiling::Stroke, m_origin, m_cache);
    } else {
        m_target.setRenderHint(QPainter::Antialiasing, false);
        set_to_content(m_target, area, Qt::NoPen, brush, m_origin);
        m_target.drawPath(area);
    }
}

void HandPaintEngine::follow_device_painter(int system_clip_margin) {
    // What the device painter draws is as opaque as it says, as Qt draws it.
    m_target.setOpacity(m_target.opacity() * painter()->opacity());
    // Intersect keeps the clip the target painter already has.
    // The system clip is the area of the widget Qt is rendering through a shared painter:
    // styles draw past it (for example the fill of a selected tab) and rely on it to cut.
    const QRegion system_clip = systemClip();
    if (!system_clip.isEmpty()) {
        QRegion clip = grown(system_clip, system_clip_margin);
        const QWidget *widget = widget_of(painter()->device());
        if (system_clip_margin > 0 && widget != nullptr) {
            clip &= drawable_rect(*widget, system_clip_margin)
                        .translated(widget_origin(*painter(), m_transform).toPoint());
        }
        m_target.setClipRegion(clip, Qt::IntersectClip);
    }
    if (m_clip) {
        m_target.setClipPath(*m_clip, Qt::IntersectClip);
    }
}

void HandPaintEngine::follow_device_painter_in_logical_coordinates() {
    follow_device_painter();
    // Combine keeps the transform the target painter already has.
    m_target.setTransform(m_transform, true);
}

namespace {

constexpr double PointsPerInch = 72.0;
// The bounds of a text in the font of the theme would take its layout, which costs as much as
// writing it. A character of any font goes no farther than these many pixel sizes of its font:
// before its place and past its advance, above and below its base line.
constexpr double MaxCharacterOverhang = 1.0;
constexpr double MaxCharacterAdvance = 2.0;
constexpr double MaxCharacterAscent = 2.0;
constexpr double MaxCharacterDescent = 1.0;

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
    const QFont &text_font = text_item.font();
    QFont font = m_theme.font();
    font.setPixelSize(pixel_size(text_font, *paintDevice()));
    // The text keeps its emphasis in the font of the theme: bold, unless the theme writes
    // bolder, and italic.
    font.setWeight(std::max(font.weight(), text_font.weight()));
    if (text_font.italic()) {
        font.setItalic(true);
    }
    // Generous bounds of the text: Qt writes it from `position` to the right, on its base line.
    const double size = font.pixelSize();
    const double length = static_cast<double>(text_item.text().size());
    const QRectF bounds(position.x() - MaxCharacterOverhang * size, position.y() - MaxCharacterAscent * size,
                        (MaxCharacterOverhang + MaxCharacterAdvance * length + MaxCharacterOverhang) * size,
                        (MaxCharacterAscent + MaxCharacterDescent) * size);
    if (is_out_of_sight(m_transform.mapRect(bounds), SmoothingReach)) {
        return;
    }
    const SavedPainter saved(m_target);
    follow_device_painter_in_logical_coordinates();
    m_target.setPen(m_theme.medium() == Medium::Chalk
                        ? chalk_pen(m_pen, ChalkGrain::Text, content_origin(*painter(), m_transform))
                        : m_pen);
    m_target.setFont(font);
    m_target.drawText(position, text_item.text());
}

}  // namespace hpq
