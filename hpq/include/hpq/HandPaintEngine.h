#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/PathCache.h>

#include <QBrush>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <cstdint>
#include <optional>
#include <vector>

class QPainter;

namespace hpq {

// Paint engine that draws each primitive it receives on a target QPainter,
// changed by a GeometryEngine and drawn as a Theme says.
class HandPaintEngine : public QPaintEngine {
public:
    // With a path cache, which must outlive the engine, the rough paths of a shape are made
    // once and kept: the theme and the geometry engine must then always be the same for that cache.
    HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine,
                    PathCache *path_cache = nullptr);

    bool begin(QPaintDevice *device) override;
    bool end() override;
    void updateState(const QPaintEngineState &state) override;
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
    void drawTextItem(const QPointF &position, const QTextItem &text_item) override;

private:
    enum class ShapeKind { Open, Closed, Rectangle };
    // What a stroke draws: the outline of a shape, or the lines of a hatched fill.
    enum class StrokeKind { Outline, Hachure };

    // Draws `shape` on the target: its fill when it is not open and the brush is set,
    // then its rough outline strokes when the pen is set.
    // A rectangle without fill is a frame: its strokes stay out of it, because what it frames
    // (the viewport of a list) is painted after it, over anything inside the rectangle.
    void draw_shape(const QPainterPath &shape, ShapeKind kind);
    // Fills `shape` (in device coordinates) on the target with the brush, as the fill style of
    // the theme says.
    void fill_shape(const QPainterPath &shape, const RoughOptions &options);
    // Whether a shape filled with `brush` gets hachures: the theme asks for them, and they
    // would show on the paper without hiding a drawing that `brush` lets through.
    bool is_hatched(const QBrush &brush) const;
    // Whether a stroke drawn with `pen` is a chalk ribbon: an area to fill, not a line.
    bool is_ribbon(const QPen &pen) const;
    // The path that draws `stroke` with a pen of `width`: itself, or its chalk ribbon, whose
    // pressure and gaps come from `stroke_seed`.
    QPainterPath inked(const QPainterPath &stroke, bool as_ribbon, double width, std::uint32_t stroke_seed) const;
    // Draws on the target, with `pen` and the medium of the theme, a stroke that inked() made.
    void draw_stroke(const QPainterPath &inked_stroke, const QPen &pen, StrokeKind kind);
    // The paths of `kind` for `shape` and `parameter`, from the path cache when there is one.
    template <typename Make>
    std::vector<QPainterPath> kept_paths(PathCache::Kind kind, const QPainterPath &shape, double parameter,
                                         Make make);
    // `brush` as the medium of the theme lays it on an area: unchanged by a pencil; with chalk,
    // rubbed on the grain, or the slate itself when it has the color of the slate.
    QBrush seen_through_medium(const QBrush &brush) const;
    bool is_slate(const QColor &color) const;
    // `pen`, drawing through the grain of the slate instead of its plain color.
    QPen chalk_pen(QPen pen, ChalkGrain grain) const;
    double target_pixel_ratio() const;
    // Draws `path` on the target with `pen` and `brush`, leaving the target state unchanged.
    // The system clip is grown by `system_clip_margin`.
    void draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush,
                        int system_clip_margin = 0);
    std::uint32_t seed(const QPainterPath &shape) const;
    // Saves the target state, then intersects the target clip with the system clip and the
    // device painter's clip.
    // What is drawn next is in device coordinates; restore the target when done.
    // The system clip (the area of the widget being rendered) is grown by `system_clip_margin`.
    void save_target_in_device_coordinates(int system_clip_margin = 0);
    // Like save_target_in_device_coordinates, then composes the device painter's world transform,
    // so that what is drawn next is in the device painter's logical coordinates.
    void save_target_in_logical_coordinates();

    // Whether nothing drawn within `margin` of `bounds` (in device coordinates) can show:
    // it is out of what the target can paint, or of the area of the widget being rendered.
    bool is_out_of_sight(const QRectF &bounds, double margin) const;
    // The farthest from a shape that its outline, drawn with the current pen, puts ink.
    double outline_overflow() const;

    QPainter &m_target;
    // The bounds of what the target painter can paint (its clips), in its coordinates; none
    // when nothing limits it.
    std::optional<QRectF> m_target_sight;
    const Theme &m_theme;
    const GeometryEngine &m_geometry_engine;
    PathCache *m_path_cache;
    QPen m_pen;
    QBrush m_brush;
    // The device painter's world transform: shapes are mapped by it to device coordinates
    // before the geometry engine deforms them, so that max_offset stays in device pixels.
    QTransform m_transform;
    // The device painter's clip, in device coordinates; used only when m_clipping is true.
    QPainterPath m_clip;
    bool m_clipping = false;
};

}  // namespace hpq
