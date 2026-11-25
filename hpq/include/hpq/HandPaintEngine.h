#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>
#include <hpq/PaintCache.h>

#include <QBrush>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QTransform>

#include <cstdint>
#include <vector>

class QPainter;

namespace hpq {

// Paint engine that draws each primitive it receives on a target QPainter,
// changed by a GeometryEngine and drawn as a Theme says.
// It smooths every line and every fill, as if the painter had the Antialiasing hint. It writes
// the text in the font of the theme, bold or italic when the font of the painter is.
// With a cache, it keeps there the rough paths and the washes that it makes, and takes them
// back when it draws the same shape again: the drawing is the same, made faster. The cache
// must serve this theme and this geometry engine only, and outlive the engine.
// A shape, a text or an image that would land out of what the target can paint (its clip, and
// the area that Qt repaints) is not drawn: a repaint of a part of a window draws only the
// shapes close to that part.
class HPQ_EXPORT HandPaintEngine : public QPaintEngine {
public:
    HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                    PaintCache *cache = nullptr);

    bool begin(QPaintDevice *device) override;
    bool end() override;
    void updateState(const QPaintEngineState &new_state) override;
    void drawPath(const QPainterPath &path) override;
    void drawRects(const QRectF *rects, int count) override;
    // The integer overloads of QPaintEngine convert to the floating point ones below.
    using QPaintEngine::drawEllipse;
    using QPaintEngine::drawLines;
    using QPaintEngine::drawPolygon;
    void drawLines(const QLineF *lines, int count) override;
    void drawEllipse(const QRectF &rect) override;
    void drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) override;
    void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) override;
    void drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                   Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    void drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &offset) override;
    Type type() const override;
    // Draws the text on the target in the theme font, at the same pixel size, in the pen color.
    // With TextInk::Invisible, draws nothing unless the pen has a HighlightedText color of the theme.
    void drawTextItem(const QPointF &position, const QTextItem &text_item) override;

private:
    enum class ShapeKind { Open, Closed, Rectangle };
    // What a stroke draws: the outline of a shape, or the lines of a hatched fill.
    enum class StrokeKind { Outline, Hachure };
    // The area that a tool marks along a stroke, filled instead of stroking it: the uneven
    // one of chalk or of a brush, or the even one of a pen wider than a pixel of the target.
    enum class Ribbon { None, Chalk, Brush, Pen };
    // How a pen draws a stroke with the medium of the theme.
    struct Tool {
        Ribbon ribbon = Ribbon::None;
        // The width, in pixels, of the line that the tool draws.
        double width = 0.0;
        // The width, in pixels, of the line where the tool presses the most.
        double widest = 0.0;
        // How the ribbon of a pen ends and turns.
        Qt::PenCapStyle cap_style = Qt::FlatCap;
        Qt::PenJoinStyle join_style = Qt::MiterJoin;
        // How far past the area of the widget being rendered the tool may draw.
        int system_clip_margin = 0;
    };
    // The seed of the shape being drawn, made when it is first asked for.
    class ShapeSeed;

    // Draws `shape` on the target: its fill when it is not open and the brush is set,
    // then its rough outline strokes when the pen is set.
    // The drawing depends on the shape and on its place in the content of the widget being
    // painted, not on where that content is: a view that scrolls keeps its drawing.
    // A rectangle without fill is a frame: its strokes stay out of it, because what it frames
    // (the viewport of a list) is painted after it, over anything inside the rectangle.
    void draw_shape(const QPainterPath &shape, ShapeKind kind);
    // Fills `shape` on the target with the brush, as the fill style of the theme says.
    // Here and below, a shape, an area or a stroke is in the coordinates of the content: in
    // device pixels, counted from m_origin.
    void fill_shape(const QPainterPath &shape, ShapeKind kind, const RoughOptions &options, ShapeSeed &shape_seed);
    // The paths of `kind` that `tool` draws for `shape`: the ones `make` returns, or the ones
    // it returned for the same shape and the same tool when the engine has a cache.
    template <typename Make>
    std::vector<QPainterPath> kept_paths(PaintCache::PathKind kind, const QPainterPath &shape, const Tool &tool,
                                         Make make);
    // Lays `brush` on `area` as the medium of the theme does. `is_band` tells that the area
    // is a rectangle without outline, which watercolor paints in one stroke of a flat brush.
    void lay_on_target(const QPainterPath &area, const QBrush &brush, bool is_band);
    // Whether the medium of the theme lays `brush` as a wash of watercolor: a plain color
    // that is not the one of the paper.
    bool is_wash(const QBrush &brush) const;
    // Whether a shape filled with `brush` gets hachures: the theme asks for them, and they
    // would show on the paper without hiding a drawing that `brush` lets through.
    bool is_hatched(const QBrush &brush) const;
    // How `pen` draws a stroke of `kind` with the medium of the theme.
    Tool tool_of(const QPen &pen, StrokeKind kind) const;
    // How far past a shape, in device pixels, what is drawn for it can put ink: its outline
    // drawn by `outline_tool` when it has one, and its fill when it is `filled`.
    double ink_reach(const Tool *outline_tool, bool filled) const;
    // Whether nothing drawn within `margin` pixels of `bounds`, in device coordinates, can
    // show: the target cannot paint there. What is out of sight is not drawn at all.
    bool is_out_of_sight(const QRectF &bounds, double margin) const;
    // Whether the target strokes a line of `pen` in one pass: the line is at most one of its pixels wide.
    bool is_thin(const QPen &pen) const;
    // What `tool` marks along `stroke`: its ribbon, or the stroke itself when it has none.
    // `stroke_seed` gives a chalk stroke its pressure and its gaps, and a brush stroke its pressure.
    QPainterPath marked(const QPainterPath &stroke, const Tool &tool, std::uint32_t stroke_seed) const;
    // Draws on the target what `tool` marked along a stroke, with `pen` and the medium of the theme.
    // A watercolor stroke has a smooth edge, and only darkens what is under it.
    void draw_marked(const QPainterPath &marked_stroke, const QPen &pen, const Tool &tool);
    // `brush` as the medium of the theme lays it on an area: unchanged by a pencil; with chalk,
    // rubbed on the grain, or the slate itself when it has the color of the slate; with
    // watercolor, the grainy paper when it has the color of the paper.
    QBrush seen_through_medium(const QBrush &brush) const;
    // Whether the ink of the theme shows a text written in `color`: any text with a visible
    // ink; with an invisible ink, only the text of a selection.
    bool shows_text(const QColor &color) const;
    // Whether `color` is the one of what the theme draws on: its slate or its paper.
    bool is_ground(const QColor &color) const;
    // `pen`, drawing through the grain of the slate instead of its plain color. The grain is
    // moved to `origin`, the origin of the content: it scrolls with the content.
    QPen chalk_pen(QPen pen, ChalkGrain grain, const QPointF &origin) const;
    // Where the origin of the widget being painted is on the device; the origin of the device
    // when the painter does not paint a widget.
    QPointF widget_origin() const;
    // Where the origin of the content of the widget being painted is on the device: the origin
    // of the widget, moved back by how far it scrolled its content when it is the viewport of a
    // scroll area.
    QPointF content_origin() const;
    // The origin of `logical_line`: the one of the content, but along a horizontal or vertical
    // line that crosses the whole widget. A view draws the lines of its grid across what it
    // shows, not across its whole content: placed along its length in the widget, such a line
    // does not change when the view scrolls.
    QPointF line_origin(const QPainterPath &logical_line) const;
    double target_pixel_ratio() const;
    // Draws `path` on the target with `pen` and `brush`, leaving the target state unchanged.
    // It is always smoothed. Without a pen, `path` is an area of `tiling`, filled in pieces.
    // The system clip is grown by `system_clip_margin`.
    void draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush, Tiling tiling,
                        int system_clip_margin = 0);
    // Fills `area` on the target with `brush` as a paint brush leaves watercolor: with a smooth
    // edge, and as a glaze that darkens what is under it, when the target can do that.
    void glaze_on_target(const QPainterPath &area, const QBrush &brush, int system_clip_margin);
    // Fills `area` with `brush` and the state of the target, which it moves to m_origin: on the
    // raster engine, which smooths a large path slowly, in the pieces of `tiling`.
    void fill_in_pieces(const QPainterPath &area, const QBrush &brush, Tiling tiling);
    // The pieces of `area` for a target that maps it to the device with `to_device`: those
    // that reach the `paintable` pixels, and maybe others. With a cache, they are kept there.
    std::vector<QPainterPath> pieces_of(const QPainterPath &area, const QTransform &to_device, const QRect &paintable,
                                        Tiling tiling);
    // Moves the target to m_origin, and gives it `pen` and `brush`, that are laid on `path`:
    // the target then draws `path`, in the coordinates of the content, as it would draw `path`
    // moved to m_origin.
    void set_target_to_content(const QPainterPath &path, const QPen &pen, const QBrush &brush);
    // Saves the target state, then intersects the target clip with the system clip and the
    // device painter's clip.
    // What is drawn next is in device coordinates; restore the target when done.
    // The system clip (the area of the widget being rendered) is grown by `system_clip_margin`,
    // but not past the viewport of a scroll area that shows the widget.
    void save_target_in_device_coordinates(int system_clip_margin = 0);
    // Like save_target_in_device_coordinates, then composes the device painter's world transform,
    // so that what is drawn next is in the device painter's logical coordinates.
    void save_target_in_logical_coordinates();

    QPainter &m_target;
    const Theme &m_theme;
    const GeometryEngine &m_geometry_engine;
    // Where the paths and washes made for a shape are kept; null when they are made at each drawing.
    PaintCache *m_cache;
    QPen m_pen;
    QBrush m_brush;
    // The device painter's world transform: shapes are mapped by it to device coordinates
    // before the geometry engine deforms them, so that max_offset stays in device pixels.
    QTransform m_transform;
    // The content_origin() of the shape being drawn.
    QPointF m_origin;
    // The device painter's clip, in device coordinates; used only when m_clipping is true.
    QPainterPath m_clip;
    bool m_clipping = false;
};

}  // namespace hpq
