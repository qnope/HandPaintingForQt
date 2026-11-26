#include <hpq/HandPaintDevice.h>
#include <hpq/HandPaintEffect.h>

#include <QAbstractScrollArea>
#include <QEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QPointer>
#include <QRegion>
#include <QScrollArea>
#include <QScrollBar>
#include <QTransform>
#include <QWidget>

#include <cmath>
#include <utility>

namespace hpq {

namespace {

// How far, in pixels, past what Qt repaints the widgets are rendered: a line up to about 6
// pixels wide, drawn with chalk or a brush past the edge of a widget, stays within it.
constexpr int RenderMargin = 14;

// The area that Qt repaints with `painter`, in its logical coordinates; empty when nothing
// limits it.
QRect repainted_area(const QPainter &painter) {
    const QRegion system_clip = painter.paintEngine()->systemClip();
    const QTransform to_device = painter.deviceTransform();
    if (system_clip.isEmpty() || !to_device.isInvertible()) {
        return QRect();
    }
    return to_device.inverted().mapRect(QRectF(system_clip.boundingRect())).toAlignedRect();
}

// The widgets of `widget` that Qt paints: its children that are widgets, shown, and not windows.
template <typename Visit>
void for_each_painted_child(const QWidget &widget, Visit visit) {
    for (const QObject *child : widget.children()) {
        const auto *child_widget = qobject_cast<const QWidget *>(child);
        if (child_widget != nullptr && !child_widget->isHidden() && !child_widget->isWindow()) {
            visit(*child_widget);
        }
    }
}

// Whether what `widget` draws may depend on the part of it that it is asked to paint, so that
// it must be painted whole to draw the same shapes at every repaint:
// - a widget without children, whose painting the theme cannot know: a table draws its grid
//   lines across the part it paints, a graphics view fills its background there;
// - the viewport of a view, or of any scroll area but the plain one, which only holds a widget;
// - a widget that fills its background, part by part, with a color that the theme may draw
//   in hachures or washes: only the paper and the slate look the same filled in parts.
// A widget that holds others and draws a frame or the paper around them looks the same painted
// in parts.
bool paints_whole(const QWidget &widget, const QPalette &theme_palette) {
    const auto *scroll_area = qobject_cast<const QAbstractScrollArea *>(widget.parentWidget());
    if (scroll_area != nullptr && scroll_area->viewport() == &widget &&
        qobject_cast<const QScrollArea *>(scroll_area) == nullptr) {
        return true;
    }
    if (widget.autoFillBackground()) {
        const QColor background = widget.palette().color(widget.backgroundRole());
        if (background != theme_palette.color(QPalette::Active, QPalette::Window) &&
            background != theme_palette.color(QPalette::Active, QPalette::Base)) {
            return true;
        }
    }
    bool has_children = false;
    for_each_painted_child(widget, [&has_children](const QWidget &) { has_children = true; });
    return !has_children;
}

// Adds to `region`, in the coordinates of `source`, the whole area of each widget under
// `widget` that touches `area` and must be painted whole. `visible` is the part of `widget`
// that its ancestors let show.
void add_whole_widgets(const QWidget &widget, const QWidget &source, const QRect &area, const QRect &visible,
                       const QPalette &theme_palette, QRegion &region) {
    for_each_painted_child(widget, [&](const QWidget &child) {
        const QRect child_area = QRect(child.mapTo(&source, QPoint()), child.size()).intersected(visible);
        if (!child_area.intersects(area)) {
            return;
        }
        if (paints_whole(child, theme_palette)) {
            region += child_area;
        } else {
            add_whole_widgets(child, source, area, child_area, theme_palette, region);
        }
    });
}

// How far past what Qt repaints the widgets are rendered: past their edges, what they draw
// reaches no farther.
int render_margin(const Theme &theme) {
    return RenderMargin + static_cast<int>(std::ceil(theme.rough_options.max_offset));
}

// Repaints a scroll area whole, with its frame, its headers, its scroll bars and what they
// draw past its edges, each time it scrolls and each time a scroll bar changes, for as long as
// it lives. Qt moves the pixels already painted and repaints only what comes into view, but
// the theme does not draw a scrolled view as it was moved: the slate and the paper stay where
// they are, a line along the scroll keeps its wobble, the handle of a scroll bar is drawn a
// little past the bar.
class ScrollAreaRepainter : public QObject {
public:
    // `area` is `source` or one of its descendants.
    ScrollAreaRepainter(QWidget &source, QAbstractScrollArea &area, int margin, QObject &parent)
        : QObject(&parent), m_source(&source), m_area(&area), m_margin(margin) {
        for (QScrollBar *bar : {area.horizontalScrollBar(), area.verticalScrollBar()}) {
            connect(bar, &QAbstractSlider::valueChanged, this, [this] { repaint(); });
            // The handle changes its length.
            connect(bar, &QAbstractSlider::rangeChanged, this, [this] { repaint(); });
            // Shown or hidden once the area lays out its scroll bars again.
            bar->installEventFilter(this);
        }
    }

    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
            repaint();
        }
        return false;
    }

private:
    void repaint() const {
        if (m_source.isNull() || m_area.isNull()) {
            return;
        }
        if (m_area.data() == m_source.data()) {
            m_source->update();
        } else if (m_source->isAncestorOf(m_area)) {
            // The parent of the area would cut the margin at its own edges.
            const QRect area(m_area->mapTo(m_source, QPoint()), m_area->size());
            m_source->update(area.adjusted(-m_margin, -m_margin, m_margin, m_margin));
        }
    }

    QPointer<QWidget> m_source;
    QPointer<QAbstractScrollArea> m_area;
    int m_margin;
};

}  // namespace

HandPaintEffect::HandPaintEffect(Theme theme, QObject *parent)
    : QGraphicsEffect(parent), m_theme(std::move(theme)) {}

void HandPaintEffect::draw(QPainter *painter) {
    HandPaintDevice device(*painter, m_theme, m_cache);
    QPainter hand_painter(&device);
    // drawSource() renders the whole source widget, and its children, whatever Qt repaints: a
    // hover on a button would paint every widget of the window again. The painter is not the
    // one of the paint context, so the effect is not applied again inside the render.
    auto *widget = qobject_cast<QWidget *>(parent());
    const QRect repainted = repainted_area(*painter);
    if (widget == nullptr || widget->graphicsEffect() != this || repainted.isEmpty()) {
        drawSource(&hand_painter);
        return;
    }
    // Only the widgets close enough to what Qt repaints to draw there, each painted whole or in
    // a part that does not change what it draws.
    const int margin = render_margin(m_theme);
    const QRect near_repainted = repainted.adjusted(-margin, -margin, margin, margin).intersected(widget->rect());
    if (near_repainted.isEmpty()) {
        return;
    }
    if (near_repainted == widget->rect()) {
        drawSource(&hand_painter);
        return;
    }
    QRegion rendered(near_repainted);
    add_whole_widgets(*widget, *widget, near_repainted, widget->rect(), m_theme.palette, rendered);
    // Rendering starts at the given offset: the top left corner of the rendered region stays in its place.
    widget->render(&hand_painter, rendered.boundingRect().topLeft(), rendered);
}

void HandPaintEffect::sourceChanged(ChangeFlags flags) {
    auto *widget = qobject_cast<QWidget *>(parent());
    if (!flags.testFlag(SourceAttached) || widget == nullptr || widget->graphicsEffect() != this) {
        return;
    }
    const int margin = render_margin(m_theme);
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget)) {
        new ScrollAreaRepainter(*widget, *area, margin, *this);
    }
    for (QAbstractScrollArea *area : widget->findChildren<QAbstractScrollArea *>()) {
        new ScrollAreaRepainter(*widget, *area, margin, *this);
    }
}

}  // namespace hpq
