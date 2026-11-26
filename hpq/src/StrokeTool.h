#pragma once

#include <hpq/FwdDecls.h>

#include <QPainterPath>
#include <QPen>

#include <cstdint>

class QPainter;

namespace hpq {

// What a stroke draws: the outline of a shape, or the lines of a hatched fill.
enum class StrokeKind { Outline, Hachure };

// The area that a tool marks along a stroke, filled instead of stroking it: the uneven
// one of chalk or of a brush, or the even one of a pen wider than a pixel of the target.
enum class Ribbon { None, Chalk, Brush, Pen };

// How a pen draws a stroke with the medium of a theme.
struct StrokeTool {
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

// How `pen` draws a stroke of `kind` with the medium of `theme`, on the `target` painter.
StrokeTool stroke_tool(const QPen &pen, StrokeKind kind, const Theme &theme, const QPainter &target);

// What `tool` marks along `stroke`: its ribbon, or the stroke itself when it has none.
// `stroke_seed` gives a chalk stroke its pressure and its gaps, and a brush stroke its pressure.
QPainterPath marked(const QPainterPath &stroke, const StrokeTool &tool, std::uint32_t stroke_seed);

// The number of `tool` in a PaintCache: two tools that mark the same stroke of the same width
// in two ways have two numbers.
int cache_number(const StrokeTool &tool);

}  // namespace hpq
