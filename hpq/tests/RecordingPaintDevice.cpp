#include "RecordingPaintDevice.h"

#include <QPixmap>
#include <QTextItem>

namespace hpq::test {

RecordingPaintEngine::RecordingPaintEngine() : QPaintEngine(QPaintEngine::AllFeatures) {}

std::vector<DrawCall> RecordingPaintEngine::fills() const { return calls_where(&DrawCall::is_fill); }

std::vector<DrawCall> RecordingPaintEngine::strokes() const { return calls_where(&DrawCall::is_stroke); }

std::vector<DrawCall> RecordingPaintEngine::calls_where(bool (DrawCall::*predicate)() const) const {
    std::vector<DrawCall> result;
    for (const DrawCall &call : m_calls) {
        if ((call.*predicate)()) {
            result.push_back(call);
        }
    }
    return result;
}

bool RecordingPaintEngine::begin(QPaintDevice *) { return true; }

bool RecordingPaintEngine::end() { return true; }

void RecordingPaintEngine::updateState(const QPaintEngineState &state) {
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
    if (flags & DirtyClipEnabled) {
        m_clip_enabled = state.isClipEnabled();
    }
    if (flags & (DirtyClipPath | DirtyClipRegion)) {
        m_clip_enabled = state.clipOperation() != Qt::NoClip;
        m_clip_path = QPainterPath();
        if (flags & DirtyClipPath) {
            m_clip_path = state.clipPath();
        } else {
            m_clip_path.addRegion(state.clipRegion());
        }
    }
}

DrawCall RecordingPaintEngine::call_with_state(DrawCall::Kind kind) const {
    DrawCall call;
    call.kind = kind;
    call.pen = m_pen;
    call.brush = m_brush;
    call.transform = m_transform;
    call.clip_enabled = m_clip_enabled;
    call.clip_path = m_clip_path;
    return call;
}

void RecordingPaintEngine::drawPath(const QPainterPath &path) {
    DrawCall call = call_with_state(DrawCall::Kind::Path);
    call.path = path;
    m_calls.push_back(call);
}

void RecordingPaintEngine::drawRects(const QRectF *rects, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath path;
        path.addRect(rects[index]);
        drawPath(path);
    }
}

void RecordingPaintEngine::drawLines(const QLineF *lines, int count) {
    for (int index = 0; index < count; ++index) {
        QPainterPath path(lines[index].p1());
        path.lineTo(lines[index].p2());
        drawPath(path);
    }
}

void RecordingPaintEngine::drawEllipse(const QRectF &rect) {
    QPainterPath path;
    path.addEllipse(rect);
    drawPath(path);
}

void RecordingPaintEngine::drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) {
    QPolygonF polygon;
    for (int index = 0; index < count; ++index) {
        polygon << points[index];
    }
    QPainterPath path;
    path.addPolygon(polygon);
    if (mode != PolylineMode) {
        path.closeSubpath();
    }
    path.setFillRule(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
    drawPath(path);
}

void RecordingPaintEngine::drawTextItem(const QPointF &position, const QTextItem &text_item) {
    DrawCall call = call_with_state(DrawCall::Kind::Text);
    call.text = text_item.text();
    call.font = text_item.font();
    call.position = position;
    m_calls.push_back(call);
}

void RecordingPaintEngine::drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) {
    drawImage(rect, pixmap.toImage(), source_rect, Qt::AutoColor);
}

void RecordingPaintEngine::drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                                     Qt::ImageConversionFlags) {
    DrawCall call = call_with_state(DrawCall::Kind::Pixmap);
    call.rect = rect;
    call.image = image;
    call.source_rect = source_rect;
    m_calls.push_back(call);
}

QPaintEngine::Type RecordingPaintEngine::type() const { return QPaintEngine::User; }

int RecordingPaintDevice::metric(PaintDeviceMetric metric) const {
    constexpr int Dpi = 96;
    switch (metric) {
    case PdmWidth:
    case PdmHeight:
        return Size;
    case PdmWidthMM:
    case PdmHeightMM:
        return Size * 254 / (10 * Dpi);
    case PdmNumColors:
        return 0;
    case PdmDepth:
        return 32;
    case PdmDpiX:
    case PdmDpiY:
    case PdmPhysicalDpiX:
    case PdmPhysicalDpiY:
        return Dpi;
    case PdmDevicePixelRatio:
        return 1;
    case PdmDevicePixelRatioScaled:
        return static_cast<int>(devicePixelRatioFScale());
    default:
        return QPaintDevice::metric(metric);
    }
}

}  // namespace hpq::test
