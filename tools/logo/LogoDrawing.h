#pragma once

// The drawing of the logo of hpq, for the tools that draw it: HpqLogo writes its images
// (docs/logo) and HpqTeaserVideo shows it in the teaser (docs/teaser).
//
// The logo is a drawing of 400 by 96 logical pixels, made of four parts: the box of a check
// box and its check (the mark), the name of the library and its underline. A tool draws them
// with hand_drawn_image and the theme that logo_theme returns: the same code draws the same
// logo everywhere.

#include <hpq/HandPaintDevice.h>
#include <hpq/Themes.h>

#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QRectF>
#include <QSize>

#include <cstdint>

namespace hpq::logo {

// Every run draws the same wobble.
constexpr std::uint32_t Seed = 20261126;

// The sizes of the drawings, in logical pixels: the mark alone, and the mark with the name.
constexpr QSize MarkSize(96, 96);
constexpr QSize LogoSize(400, 96);

struct Colors {
    QColor box_line;
    QColor box_fill;
    QColor check;
    QColor name;
    QColor underline;
};

// The theme that `make` returns with hatched fills, drawn by the hand of the logo.
inline Theme logo_theme(Theme (*make)(FillStyle)) {
    Theme theme = make(FillStyle::Hachure);
    theme.rough_options.seed = Seed;
    return theme;
}

// The colors of the logo in pencil, for a light page: `pencil` is logo_theme(sketch_theme).
inline Colors colors_on_paper(const Theme &pencil) {
    return {QColor("#2a6fdb"), QColor("#7fb3e6"), QColor("#e67e22"), pencil.palette.color(QPalette::WindowText),
            QColor("#7d3c98")};
}

// The colors of the logo in chalk, for a dark page: `chalk` is logo_theme(slate_theme).
inline Colors colors_on_slate(const Theme &chalk) {
    return {QColor("#7fb8f0"), QColor("#7fb8f0"), QColor("#ffb347"), chalk.palette.color(QPalette::WindowText),
            QColor("#c39bd3")};
}

// The box of the mark: a check box, the simplest of widgets. The paper or the slate of the
// theme shows between its hachures.
inline void draw_box(QPainter &painter, const Colors &colors) {
    painter.setPen(QPen(colors.box_line, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(colors.box_fill);
    painter.drawRect(QRectF(10, 22, 62, 62));
}

// The check of the mark: a stroke that leaves the box.
inline void draw_check(QPainter &painter, const Colors &colors) {
    QPainterPath check;
    check.moveTo(24, 50);
    check.lineTo(39, 68);
    check.lineTo(84, 12);
    painter.setPen(QPen(colors.check, 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(check);
}

// The mark: the box, checked.
inline void draw_mark(QPainter &painter, const Colors &colors) {
    draw_box(painter, colors);
    draw_check(painter, colors);
}

// The name of the library, at the right of the mark.
inline void draw_name(QPainter &painter, const Colors &colors) {
    QFont font = painter.font();
    font.setPixelSize(30);
    painter.setFont(font);
    painter.setPen(colors.name);
    painter.drawText(QRectF(104, 14, 290, 44), Qt::AlignLeft | Qt::AlignVCenter, "Hand Painting for Qt");
}

// The line that underlines the name, by hand.
inline void draw_underline(QPainter &painter, const Colors &colors) {
    QPainterPath underline;
    underline.moveTo(106, 66);
    underline.cubicTo(170, 60, 290, 74, 388, 64);
    painter.setPen(QPen(colors.underline, 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(underline);
}

// An image of `size` logical pixels, filled with `background`, on which `draw` draws by hand
// with `theme`. The image has `pixel_ratio` pixels for a logical pixel: the logo is drawn as a
// small widget on a screen of a high pixel density, and the wobble and the hachures of a theme
// keep the size they have next to a button, however large the image is.
template <typename Draw>
QImage hand_drawn_image(QSize size, double pixel_ratio, const QColor &background, const Theme &theme, Draw draw) {
    QImage image(size * pixel_ratio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(pixel_ratio);
    image.fill(background);
    {
        QPainter target(&image);
        HandPaintDevice device(target, theme);
        QPainter painter(&device);
        draw(painter);
    }
    return image;
}

}  // namespace hpq::logo
