#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>
#include <hpq/PaintCache.h>
#include <hpq/Theme.h>

#include <QGraphicsEffect>

#include <memory>

namespace hpq {

// Graphics effect that draws its source (a widget and its children) through a
// HandPaintDevice, so every primitive gets the hand-painted look of the theme.
// It keeps what was made to draw the shapes of its last repaints: a shape that did not change
// since is drawn again without being made rough again.
// When Qt repaints a part of the source, the effect renders only the widgets close to that
// part. Each widget without children, each viewport of a view and each widget that fills its
// background with a color of its own is painted whole: what it draws may depend on the part
// that it paints. A widget that holds others is asked to paint only a part of itself: it must
// draw the same shapes whatever that part, as the containers of Qt do (a frame, the paper of
// the window).
// Installed on the widget it is a child of, it repaints whole each scroll area that the widget
// holds then, when the area scrolls or a scroll bar changes: Qt would move the pixels already
// painted, but the theme does not draw a scrolled view as it was moved.
class HPQ_EXPORT HandPaintEffect : public QGraphicsEffect {
    Q_OBJECT

public:
    HandPaintEffect(Theme theme,
                    std::shared_ptr<const GeometryEngine> geometry_engine,
                    QObject *parent = nullptr);

protected:
    void draw(QPainter *painter) override;
    void sourceChanged(ChangeFlags flags) override;

private:
    Theme m_theme;
    std::shared_ptr<const GeometryEngine> m_geometry_engine;
    PaintCache m_cache;
};

}  // namespace hpq
