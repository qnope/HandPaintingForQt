#pragma once

#include <QBrush>
#include <QColor>
#include <QImage>

namespace hpq {

// How chalk takes on the grain of a slate.
enum class ChalkGrain {
    // The tip of the chalk: mostly covered, with pinholes and a few bare patches.
    Stroke,
    // The tip of the chalk writing small letters: only the pinholes, so that the text stays legible.
    Text,
    // The side of the chalk rubbed over an area: half covered, in diagonal streaks.
    Fill,
};

// How much chalk each pixel of the slate takes, as a Format_Alpha8 image that tiles without
// a seam. The same image is returned by every call.
QImage chalk_grain(ChalkGrain grain);

// A brush that paints `color` through the grain, and has it as its color. It is anchored to
// the origin of the painter: who draws with it moves it to the origin of what is drawn, so
// that the holes scroll with it.
// `device_pixel_ratio` is the one of the device painted on: the grain keeps its size in logical
// pixels, and its image has the resolution of the device, so that it is drawn as it is.
QBrush chalk_brush(const QColor &color, ChalkGrain grain, double device_pixel_ratio);

// A brush that paints the slate itself: `color` under a thin, uneven veil of chalk dust.
QBrush slate_brush(const QColor &color, double device_pixel_ratio);

}  // namespace hpq
