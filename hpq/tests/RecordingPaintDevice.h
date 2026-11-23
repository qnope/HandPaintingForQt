#pragma once

#include <QBrush>
#include <QFont>
#include <QImage>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QTransform>

#include <vector>

namespace hpq::test {

// One draw call received by a RecordingPaintEngine, with the painter state at that time.
struct DrawCall {
    enum class Kind { Path, Text, Pixmap };

    Kind kind = Kind::Path;
    // Kind::Path: the shape, in the coordinates given by the painter (see transform).
    QPainterPath path;
    QPen pen;
    QBrush brush;
    QTransform transform;
    bool clip_enabled = false;
    QPainterPath clip_path;
    // Kind::Text.
    QString text;
    QFont font;
    QPointF position;
    // Kind::Pixmap: the target rectangle and the source part of the image.
    QRectF rect;
    QImage image;
    QRectF source_rect;

    bool is_fill() const { return kind == Kind::Path && brush.style() != Qt::NoBrush && pen.style() == Qt::NoPen; }
    bool is_stroke() const { return kind == Kind::Path && pen.style() != Qt::NoPen && brush.style() == Qt::NoBrush; }
};

// Whether the two paths have the same elements, less than a thousandth of a pixel apart.
bool are_close(const QPainterPath &first, const QPainterPath &second);

// Paint engine that does not draw: it records each draw call it receives.
// Every primitive becomes a Kind::Path call, so tests compare paths only.
class RecordingPaintEngine : public QPaintEngine {
public:
    RecordingPaintEngine();

    const std::vector<DrawCall> &calls() const { return m_calls; }
    std::vector<DrawCall> fills() const;
    std::vector<DrawCall> strokes() const;

    bool begin(QPaintDevice *device) override;
    bool end() override;
    void updateState(const QPaintEngineState &state) override;
    void drawPath(const QPainterPath &path) override;
    void drawRects(const QRectF *rects, int count) override;
    void drawLines(const QLineF *lines, int count) override;
    void drawEllipse(const QRectF &rect) override;
    void drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) override;
    void drawTextItem(const QPointF &position, const QTextItem &text_item) override;
    void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &source_rect) override;
    void drawImage(const QRectF &rect, const QImage &image, const QRectF &source_rect,
                   Qt::ImageConversionFlags flags) override;
    Type type() const override;

private:
    DrawCall call_with_state(DrawCall::Kind kind) const;
    std::vector<DrawCall> calls_where(bool (DrawCall::*predicate)() const) const;

    QPen m_pen;
    QBrush m_brush;
    QTransform m_transform;
    bool m_clip_enabled = false;
    QPainterPath m_clip_path;
    std::vector<DrawCall> m_calls;
};

// Paint device of a fixed size (200 x 200 at 96 dpi) whose engine records the draw calls.
class RecordingPaintDevice : public QPaintDevice {
public:
    static constexpr int Size = 200;

    QPaintEngine *paintEngine() const override { return &m_engine; }
    const RecordingPaintEngine &engine() const { return m_engine; }

protected:
    int metric(PaintDeviceMetric metric) const override;

private:
    mutable RecordingPaintEngine m_engine;
};

}  // namespace hpq::test
