#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/PathCache.h>
#include <hpq/Theme.h>

#include <QGraphicsEffect>

#include <memory>

namespace hpq {

// Graphics effect that draws its source (a widget and its children) through a
// HandPaintDevice, so every primitive gets the hand-painted look of the theme.
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
    // The rough paths of the shapes of the last repaints: the theme and the geometry engine
    // of an effect never change, so the paths of a shape do not either.
    PathCache m_path_cache;
};

}  // namespace hpq
