#pragma once

#include <hpq/Export.h>
#include <hpq/FwdDecls.h>
#include <hpq/Theme.h>

#include <QGraphicsEffect>

#include <memory>

namespace hpq {

// Graphics effect that draws its source (a widget and its children) through a
// HandPaintDevice, so every primitive gets the hand-painted look of the theme.
class HPQ_EXPORT HandPaintEffect : public QGraphicsEffect {
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
};

}  // namespace hpq
