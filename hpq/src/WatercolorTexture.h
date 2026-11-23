#pragma once

#include <QBrush>
#include <QColor>
#include <QImage>

namespace hpq {

// How unevenly a wash settles on the paper, as a Format_Grayscale8 image that tiles without a
// seam, at the resolution of a device of `device_pixel_ratio`: 128 where the wash keeps the
// pigment its brush laid, more where pigment gathers (in the hollows of the grain of the
// paper, and where the wash stayed wet longer), less where it is washed away.
// The image is anchored to the origin of the painter, like the paper. The same image is
// returned by every call for a device pixel ratio.
QImage pigment_settling(double device_pixel_ratio);

// The level of a pixel of pigment_settling, from -1 (washed away) to 1 (gathered).
double settling_level(uchar settling);

// The color of `pigment` on white paper when the wash is `density` times as thick as the
// one that gives the color of the pigment: paler below 1, darker and more saturated above,
// and white at 0. Light goes through the layer twice: each channel is raised to `density`.
QRgb laid_pigment(const QColor &pigment, double density);

// A brush that paints the paper itself: `color` with the grain of a cold pressed paper.
// It is anchored to the origin of the painter: the grain stays at the same places of the paper.
QBrush paper_brush(const QColor &color, double device_pixel_ratio);

// A brush that paints the line of a paint brush loaded with `color`: the pigment settles
// unevenly along the line, darker here and paler there. It has `color` as its color, and is
// anchored to the origin of the painter: who draws a line with it moves it to the line.
QBrush pigment_brush(const QColor &color, double device_pixel_ratio);

}  // namespace hpq
