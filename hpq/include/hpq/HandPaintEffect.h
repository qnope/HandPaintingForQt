#pragma once

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
class HandPaintEffect : public QGraphicsEffect {
    Q_OBJECT

public:
    HandPaintEffect(Theme theme,
                    std::shared_ptr<const GeometryEngine> geometry_engine,
                    QObject *parent = nullptr);

protected:
    void draw(QPainter *painter) override;

private:
    Theme m_theme;
    std::shared_ptr<const GeometryEngine> m_geometry_engine;
    PaintCache m_cache;
};

}  // namespace hpq
