#include "StrokeTool.h"

#include "StrokeRibbon.h"

#include <hpq/Medium.h>
#include <hpq/RoughOptions.h>
#include <hpq/Theme.h>

#include <QPaintEngine>
#include <QPainter>
#include <QPainterPathStroker>
#include <QTransform>

#include <algorithm>
#include <cmath>

namespace hpq {

namespace {

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

// True when `pen` draws a plain line of one color, which chalk or a brush can follow with
// uneven pressure.
bool is_plain_line(const QPen &pen) {
    return pen.style() == Qt::SolidLine && pen.brush().style() == Qt::SolidPattern;
}

// Whether `target` strokes a line of `pen` in one pass: the line is at most one of its pixels wide.
bool is_thin(const QPen &pen, const QPainter &target) {
    if (pen.isCosmetic()) {
        return true;
    }
    const QTransform to_device = target.deviceTransform();
    return pen.widthF() * std::hypot(to_device.m11(), to_device.m12()) <= 1.0;
}

}  // namespace

StrokeTool stroke_tool(const QPen &pen, StrokeKind kind, const Theme &theme, const QPainter &target) {
    const Medium medium = theme.medium();
    const bool is_outline = kind == StrokeKind::Outline;
    const bool is_chalk = medium == Medium::Chalk;
    const bool is_watercolor = medium == Medium::Watercolor;
    // A brush follows a plain line; a dashed line is drawn by its pen, as a pencil draws it.
    const bool is_brush = (medium == Medium::Ink || is_watercolor) && is_plain_line(pen);
    StrokeTool tool;
    if (is_brush) {
        tool.ribbon = Ribbon::Brush;
    } else if (is_chalk && is_plain_line(pen)) {
        tool.ribbon = Ribbon::Chalk;
    } else if (!is_chalk && target.paintEngine()->type() == QPaintEngine::Raster && is_plain_line(pen) &&
               !is_thin(pen, target) && (pen.joinStyle() != Qt::MiterJoin || pen.miterLimit() == DefaultMiterLimit)) {
        // The raster engine turns a wide line into an area each time it strokes it, and smooths
        // that area in one piece, slowly: here the area is made once, kept with the stroke when
        // there is a cache, and filled in pieces.
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
    tool.widest = tool.width * (is_chalk ? ChalkWidestPressure : is_brush ? BrushWidestPressure : 1.0);
    tool.system_clip_margin =
        is_outline ? static_cast<int>(std::ceil(theme.rough_options().max_offset + tool.widest)) : 0;
    return tool;
}

QPainterPath marked(const QPainterPath &stroke, const StrokeTool &tool, std::uint32_t stroke_seed) {
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

// The ribbon, the cap style and the join style, each in bits of its own.
int cache_number(const StrokeTool &tool) {
    return static_cast<int>(tool.ribbon) | (static_cast<int>(tool.cap_style) << 8) |
           (static_cast<int>(tool.join_style) << 16);
}

}  // namespace hpq
