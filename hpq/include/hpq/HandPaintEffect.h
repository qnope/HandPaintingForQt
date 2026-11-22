#pragma once

#include <hpq/FwdDecls.h>
#include <hpq/Theme.h>

#include <QGraphicsEffect>
#include <QPointer>
#include <QWidget>

class QPainter;

namespace hpq {

// A graphics effect that draws its source widget hand-painted with a theme, through a HandPaintDevice.
class HandPaintEffect : public QGraphicsEffect
{
public:
    explicit HandPaintEffect(Theme theme, QObject* parent = nullptr);

    const Theme& theme() const { return m_theme; }
    // Sets the theme and schedules a redraw of the source.
    void set_theme(Theme theme);

protected:
    void draw(QPainter* painter) override;
    void sourceChanged(ChangeFlags flags) override;

private:
    // Draws the source on the painter, without the window background of the source widget.
    void draw_source_without_background(QPainter& painter);

    Theme m_theme;
    // The widget whose graphics effect this is, if any.
    QPointer<QWidget> m_source_widget;
};

} // namespace hpq
