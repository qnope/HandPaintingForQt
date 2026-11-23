#pragma once

#include <hpq/FwdDecls.h>

#include <QBrush>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <cstdint>

class QPainter;

namespace hpq {

// Paint engine that draws each primitive it receives on a target QPainter,
// changed by a GeometryEngine and drawn as a Theme says.
class HandPaintEngine : public QPaintEngine {
public:
    HandPaintEngine(QPainter &target, const Theme &theme, const GeometryEngine &geometry_engine);

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
    // With TextInk::Invisible, draws nothing unless the pen has a HighlightedText color of the theme.
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
    void fill_shape(const QPainterPath &shape, ShapeKind kind, const RoughOptions &options,
                    std::uint32_t shape_seed);
    // Lays `brush` on `area` as the medium of the theme does. `is_band` tells that the area
    // is a rectangle without outline, which watercolor paints in one stroke of a flat brush.
    void lay_on_target(const QPainterPath &area, const QBrush &brush, bool is_band);
    // Whether the medium of the theme lays `brush` as a wash of watercolor: a plain color
    // that is not the one of the paper.
    bool is_wash(const QBrush &brush) const;
    // Whether a shape filled with `brush` gets hachures: the theme asks for them, and they
    // would show on the paper without hiding a drawing that `brush` lets through.
    bool is_hatched(const QBrush &brush) const;
    // Draws one stroke on the target with `pen` and the medium of the theme.
    // `stroke_seed` gives a chalk stroke its pressure and its gaps, and a brush stroke its pressure.
    // A watercolor stroke has a smooth edge, and only darkens what is under it.
    void draw_stroke(const QPainterPath &stroke, const QPen &pen, std::uint32_t stroke_seed, StrokeKind kind);
    // `brush` as the medium of the theme lays it on an area: unchanged by a pencil; with chalk,
    // rubbed on the grain, or the slate itself when it has the color of the slate; with
    // watercolor, the grainy paper when it has the color of the paper.
    QBrush seen_through_medium(const QBrush &brush) const;
    // Whether the ink of the theme shows a text written in `color`: any text with a visible
    // ink; with an invisible ink, only the text of a selection.
    bool shows_text(const QColor &color) const;
    // Whether `color` is the one of what the theme draws on: its slate or its paper.
    bool is_ground(const QColor &color) const;
    // `pen`, drawing through the grain of the slate instead of its plain color.
    QPen chalk_pen(QPen pen, ChalkGrain grain) const;
    double target_pixel_ratio() const;
    // Draws `path` on the target with `pen` and `brush`, leaving the target state unchanged.
    // The system clip is grown by `system_clip_margin`.
    void draw_on_target(const QPainterPath &path, const QPen &pen, const QBrush &brush,
                        int system_clip_margin = 0);
    // Fills `area` on the target with `brush` as a paint brush leaves watercolor: with a smooth
    // edge, and as a glaze that darkens what is under it, when the target can do that.
    void glaze_on_target(const QPainterPath &area, const QBrush &brush, int system_clip_margin);
    std::uint32_t seed(const QPainterPath &shape) const;
    // Saves the target state, then intersects the target clip with the system clip and the
    // device painter's clip.
    // What is drawn next is in device coordinates; restore the target when done.
    // The system clip (the area of the widget being rendered) is grown by `system_clip_margin`.
    void save_target_in_device_coordinates(int system_clip_margin = 0);
    // Like save_target_in_device_coordinates, then composes the device painter's world transform,
    // so that what is drawn next is in the device painter's logical coordinates.
    void save_target_in_logical_coordinates();

    QPainter &m_target;
    const Theme &m_theme;
    const GeometryEngine &m_geometry_engine;
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
