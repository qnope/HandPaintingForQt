#include "PathTiles.h"

#include <QList>
#include <QPolygonF>
#include <QRectF>

#include <cmath>

namespace hpq {

namespace {

// Side, in pixels of the device, of a tile.
constexpr double TileSize = 64.0;

using Polygons = QList<QPolygonF>;

// Cuts `polygon` along the line at `cut`, across x or across y, in its part before the line
// and its part after it. Where the polygon leaves a part, the part goes along the line to
// where the polygon comes back.
void cut_polygon(const QPolygonF &polygon, bool across_x, double cut, QPolygonF &before, QPolygonF &after) {
    const auto coordinate = [across_x](const QPointF &point) { return across_x ? point.x() : point.y(); };
    for (int index = 0; index < polygon.size(); ++index) {
        const QPointF &from = polygon[index];
        const QPointF &to = polygon[(index + 1) % polygon.size()];
        const double from_coordinate = coordinate(from);
        const double to_coordinate = coordinate(to);
        if (from_coordinate <= cut) {
            before.append(from);
        }
        if (from_coordinate >= cut) {
            after.append(from);
        }
        if ((from_coordinate < cut && to_coordinate > cut) || (from_coordinate > cut && to_coordinate < cut)) {
            const double t = (cut - from_coordinate) / (to_coordinate - from_coordinate);
            const QPointF crossing = from + t * (to - from);
            const QPointF on_line = across_x ? QPointF(cut, crossing.y()) : QPointF(crossing.x(), cut);
            before.append(on_line);
            after.append(on_line);
        }
    }
}

// A rectangle of the device, by its sides: a cut gives a side the coordinate of the cut, exactly.
struct Region {
    double left;
    double top;
    double right;
    double bottom;
};

// True when the side from `from` to `to` goes along a side of `region`.
bool is_along(const QPointF &from, const QPointF &to, const Region &region) {
    return (from.x() == to.x() && (from.x() == region.left || from.x() == region.right)) ||
           (from.y() == to.y() && (from.y() == region.top || from.y() == region.bottom));
}

double signed_area(const QPolygonF &polygon) {
    double twice_area = 0.0;
    for (int index = 0; index < polygon.size(); ++index) {
        const QPointF &from = polygon[index];
        const QPointF &to = polygon[(index + 1) % polygon.size()];
        twice_area += from.x() * to.y() - to.x() * from.y();
    }
    return twice_area / 2.0;
}

// True when `polygon`, the part of a polygon in `region`, fills some of it. A polygon that only
// goes along the sides of the region fills all of it, or nothing: it then goes around it, or
// back and forth along its sides.
bool fills(const QPolygonF &polygon, const Region &region) {
    if (polygon.size() < 3) {
        return false;
    }
    for (int index = 0; index < polygon.size(); ++index) {
        if (!is_along(polygon[index], polygon[(index + 1) % polygon.size()], region)) {
            return true;
        }
    }
    const double region_area = (region.right - region.left) * (region.bottom - region.top);
    return std::abs(signed_area(polygon)) > region_area / 2.0;
}

// Cuts `polygons`, inside `region`, in two across the longer side of the region, between two
// pixels, until each part fits in a tile. Leaves out the parts outside `paintable`.
void cut_in_tiles(const Polygons &polygons, const Region &region, const QRectF &paintable,
                  std::vector<Polygons> &tiles) {
    if (region.right <= paintable.left() || region.left >= paintable.right() || region.bottom <= paintable.top() ||
        region.top >= paintable.bottom()) {
        return;
    }
    const double width = region.right - region.left;
    const double height = region.bottom - region.top;
    if (width <= TileSize && height <= TileSize) {
        tiles.push_back(polygons);
        return;
    }
    const bool across_x = width >= height;
    const double cut = std::round(across_x ? (region.left + region.right) / 2.0 : (region.top + region.bottom) / 2.0);
    Region before_region = region;
    Region after_region = region;
    if (across_x) {
        before_region.right = cut;
        after_region.left = cut;
    } else {
        before_region.bottom = cut;
        after_region.top = cut;
    }
    Polygons before_parts;
    Polygons after_parts;
    for (const QPolygonF &polygon : polygons) {
        QPolygonF before;
        QPolygonF after;
        cut_polygon(polygon, across_x, cut, before, after);
        if (fills(before, before_region)) {
            before_parts.append(std::move(before));
        }
        if (fills(after, after_region)) {
            after_parts.append(std::move(after));
        }
    }
    if (!before_parts.isEmpty()) {
        cut_in_tiles(before_parts, before_region, paintable, tiles);
    }
    if (!after_parts.isEmpty()) {
        cut_in_tiles(after_parts, after_region, paintable, tiles);
    }
}

}  // namespace

std::vector<QPainterPath> path_tiles(const QPainterPath &path, const QTransform &to_device, const QRect &paintable) {
    if (paintable.isEmpty()) {
        return {};
    }
    const QRectF bounds = to_device.mapRect(path.boundingRect());
    if (bounds.width() <= TileSize && bounds.height() <= TileSize) {
        return {path};
    }
    std::vector<Polygons> tiles;
    cut_in_tiles(path.toSubpathPolygons(to_device), Region{bounds.left(), bounds.top(), bounds.right(), bounds.bottom()},
                 QRectF(paintable), tiles);
    const QTransform from_device = to_device.inverted();
    std::vector<QPainterPath> pieces;
    pieces.reserve(tiles.size());
    for (const Polygons &tile : tiles) {
        QPainterPath piece;
        piece.setFillRule(path.fillRule());
        for (const QPolygonF &polygon : tile) {
            piece.addPolygon(from_device.map(polygon));
            piece.closeSubpath();
        }
        pieces.push_back(piece);
    }
    return pieces;
}

}  // namespace hpq
